#include <lvgl.h> // graphics library
#include <TFT_eSPI.h> // (library for LCD display)
#include <PubSubClient.h> // mqtt
#include "DateTime.h"
#include "RTC_SAMD51.h" // RTC clock so that our microcontroller knows when 1970 was compared to now
#include "rpcWiFi.h" // wifi library
#include "conf.h" // custom constants
#include "Grove_Temperature_And_Humidity_Sensor.h"
#include "ArduinoJson.h" // makes json parsing for the mqtt callback 10x easier


DHT dht(DHT_PIN, DHT_TYPE);
RTC_SAMD51 rtc; // https://wiki.seeedstudio.com/Wio-Terminal-RTC/
TFT_eSPI tft = TFT_eSPI(); // tft instance

WiFiClient wifiClient;
PubSubClient client(wifiClient); // create wifi client for mqtt

// new wall of const (wall of static)
static lv_color_t buf[LV_HOR_RES_MAX * 10]; // display buffer for LVGL (defining how big of a chunk of the display LVGL can work on at once)
static uint32_t tick(void) { return millis(); } // tick func
static lv_group_t *mainGroup; // maybe there was nothing wrong with one giant group
static lv_group_t *buttons; 
static lv_obj_t *mainPage; // globalized out of desperation but we will reuse menu anyway right (copium)
static lv_obj_t *subPage; 
static lv_obj_t *current; // i'm so sorry

// Subscribers inside of LVGL, not MQTT!!
lv_subject_t temperatureSubscriber;
lv_subject_t humiditySubscriber;
lv_subject_t soundSubscriber;
lv_subject_t lightingSubscriber;

lv_obj_t *statusScreen; // page for the colour-coded message
lv_obj_t *valuesScreen; // page for the actual values
lv_obj_t *uncomfortableSelectionScreen; // page for the uncomfortable value selection

const char * mqttHost = "broker.hivemq.com";
const int port = 1883;
const char * readingsTopic = "painpatrol/readings"; // for publishing
const char * wioBoundsTopic = "painpatrol/wio/bounds"; 
const char * appBoundsTopic = "painpatrol/app/bounds";
const char * clientId = "wio-terminal";

int isConnectedToWiFi;
int isConnectedToMQTT;
int isBuzzerMuted;

unsigned long globalBuzzerTime;
unsigned long publishTime;

struct SensorMeta {
  float minVal;
  float maxVal;
  const char *tooLowMsg; // message for below minVal
  const char *tooHighMsg; // message for above maxVal
  const char *okMsg; // within threshold
};

struct SensorComfort {
  const char *sensorName;
  const char **messages;
  int messagesIndex;
};

const static int uncomfortableValues[] = {0, -1, 1}; // 0 is good, -1 is below, 1 is above, maps to the array indices
// we can then use the currentIndex and the uncomfortableValues array to modify the ranges

// currently the only way of having reusability w/ the msgs, will rethink if there's a better way when refactoring
const char *temperatureMsgs[] = {"Good", "Too cold", "Too hot"};
const char *humidityMsgs[] = {"Good", "Too dry", "Too humid"};
const char *soundMsgs[] = {"Good", "Too quiet", "Too loud"};
const char *lightingMsgs[] = {"Good", "Too dark", "Too bright"};

const char *cellLabels[] = {"Temperature", "Humidity", "Lighting", "Sound"};

static SensorMeta sensorMeta[4] = {
    {TEMP_MIN,     TEMP_MAX,     temperatureMsgs[1], temperatureMsgs[2], temperatureMsgs[0]},
    {HUMIDITY_MIN, HUMIDITY_MAX, humidityMsgs[1],  humidityMsgs[2], humidityMsgs[0]},
    {LIGHT_MIN,    LIGHT_MAX,    lightingMsgs[1], lightingMsgs[2], lightingMsgs[0]},
    {SOUND_MIN,    SOUND_MAX,    soundMsgs[1], soundMsgs[2], soundMsgs[0]},
  };


int getSensorStatus(float value, float minVal, float maxVal) {
  // a simple check if the sensor's reading is below/above/within threshold
  if (value < minVal) return -1;
  if (value > maxVal) return 1;
  return 0;
}


static void valueChangedCallback(lv_observer_t *observer, lv_subject_t *subject) {
    lv_obj_t *label = lv_observer_get_target_obj(observer);

    float value = lv_subject_get_float(subject); // sensor's value

    const char *unit = (const char *)lv_observer_get_user_data(observer); // get user data which is a void pointer (generic), convert it to a char array

    lv_label_set_text_fmt(label, "%.2f %s", value, unit);
}


void display2x2Grid(lv_obj_t *parent) { // mostly follows the example for grid in the lvgl docs
    // takes up remaining area (fraction) of space remaining in the grid
    // LV_GRID_TEMPLATE_LAST is just for checking whether it's reached the end of the row/column array
    // you can verify this in the count_tracks function in case I am wrong
    // have each cell take up the a quarter of the screen, so all cells take up the whole screen

    static int32_t cellColumns[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };
    static int32_t cellRows[] = { LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST };

    lv_obj_t *grid = lv_obj_create(parent); // create grid inside of parent object (whatever that may be)

    lv_obj_set_size(grid, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_center(grid);

    // the grid is described through the cellColumns and the cellRows
    lv_obj_set_grid_dsc_array(grid, cellColumns, cellRows);

    // styling
    lv_obj_set_style_bg_color(grid, lv_color_hex(0x000000), 0);
    lv_obj_set_style_radius(grid, 0, 0);
    lv_obj_set_style_border_color(grid, lv_color_hex(0x000000), 0);

    // get rid of padding so cells can fill the entire screen
    lv_obj_set_style_pad_left(grid, 0 , 0);
    lv_obj_set_style_pad_right(grid, 0 , 0);
    lv_obj_set_style_pad_top(grid, 0 , 0);
    lv_obj_set_style_pad_bottom(grid, 0 , 0);

    // so we don't make a new variable each time in the loop
    lv_obj_t *valueLabel;
    lv_obj_t *cellLabel;
    lv_obj_t *cell;
    char unit[4]; // 4 because degree symbol has interesting ascii value

    lv_subject_t *subscribers[4] = {&temperatureSubscriber, &humiditySubscriber, &lightingSubscriber, &soundSubscriber};
    // we want to borrow the value from the subscribers, which is why we have the & symbol
    // if we didn't do this we'd get a copy of the temperature subscriber (which then doesn't have the callback called every time, meaning we'd just
    // have one initial reading and then nothing)

    // yes it's a static value, could later make it dynamic by removing static from cellColumns and cellRows
    // but the documentation does not seem to like that, so for now it's kept
    for (int i = 0; i < SENSOR_AMOUNT; i++) {
        // get position of cell in the grid and create the cell object
        int column = i % 2;
        int row = i / 2;

        cell = lv_obj_create(grid);

        // style the cell to match both (values & status) grids
        lv_obj_set_style_bg_color(cell, lv_color_hex(0x16213E), 0);
        lv_obj_set_style_border_color(cell, lv_color_hex(0x16213E), 0);

        // set the cell's position in the grid and have it stretch to fill the entire width & height of the cell
        lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, column, 1, LV_GRID_ALIGN_STRETCH, row, 1);

        cellLabel = lv_label_create(cell);
        valueLabel = lv_label_create(cell);

        const char *unit;

        if (strcmp(cellLabels[i], "Temperature") == 0) {
          unit = "°C";
        } 
        else {
          unit = "%";
        }

        // last parameter as per docs is user_data which is of type void *, void * just means that you can use any type
        // however we then need to handle that in the callback when we use it (i.e. converting the void * to char array)
        // basically a generic if you know abt it
        lv_subject_add_observer_obj(subscribers[i], valueChangedCallback, valueLabel, (void *) unit);

        lv_label_set_text_fmt(cellLabel, "%s", cellLabels[i]);
        
        lv_obj_set_style_text_color(cellLabel, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_color(valueLabel, lv_color_hex(0xFFFFFF), 0);

        lv_obj_align(cellLabel, LV_ALIGN_TOP_MID, 0, 0); // align cell label to top mid without an offset
        lv_obj_center(valueLabel);
    }

    // reduce gap between cells
    lv_obj_set_style_pad_row(grid, 4, 0);
    lv_obj_set_style_pad_column(grid, 4, 0);
}

void buzz() {
  // prevent buzzer from buzzing too frequently

  if ((millis() - globalBuzzerTime >= BUZZER_COOLDOWN) && !isBuzzerMuted) {

    for (int i = 0; i < BUZZ_AMOUNT; i++) {
      analogWrite(WIO_BUZZER, 16);

      delay(BUZZ_TIME);

      analogWrite(WIO_BUZZER, 0);

      delay(BUZZ_TIME);
    }

    globalBuzzerTime = millis();
  }
}

// helper func for switching logic
void toggleBuzzerMute() {
  if (isBuzzerMuted) {
    isBuzzerMuted = 0;
  } else {
    isBuzzerMuted = 1;
  }
}

static void statusChangedCallback(lv_observer_t *observer, lv_subject_t *subject)
{
  lv_obj_t *valueLabel = lv_observer_get_target_obj(observer);
  SensorMeta *meta = (SensorMeta *)lv_observer_get_user_data(observer); // convert to SensorMeta so we can use its fields

  float value = lv_subject_get_float(subject); // sensor's value
  int status = getSensorStatus(value, meta->minVal, meta->maxVal); // helper func to check if it's below/above/within threshold (-1/1/0)

  lv_color_t colour;
  const char *statusMsg;

  if (status == 0)
  {
    colour = lv_color_hex(0x00FF00); // green is good
    statusMsg = meta->okMsg;
  }
  else
  {
    colour = lv_color_hex(0xFF0000); // red is bad :c
    statusMsg = (status == -1) ? meta->tooLowMsg : meta->tooHighMsg;

    buzz();
  }

  lv_label_set_text(valueLabel, statusMsg);
  lv_obj_set_style_text_color(valueLabel, colour, 0);
}


void displayStatusGrid(lv_obj_t *parent)
{ // for now it's just a close cousin of display2x2Grid, the differences are
  // colour-coded messages for status (uses diff callback fun), no units, and new cell styling
  // likely can be refactored into one page
  // now there are two pages that can be changed by moving the joystick left/right

  static int32_t cellColumns[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static int32_t cellRows[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

  lv_obj_t *grid = lv_obj_create(parent); // create grid inside of parent object (whatever that may be)

  lv_obj_set_size(grid, LV_HOR_RES_MAX, LV_VER_RES_MAX);
  lv_obj_center(grid);

  // the grid is described through the cellColumns and the cellRows
  lv_obj_set_grid_dsc_array(grid, cellColumns, cellRows);

  // styling
  lv_obj_set_style_bg_color(grid, lv_color_hex(0x000000), 0);
  lv_obj_set_style_radius(grid, 0, 0);
  lv_obj_set_style_border_color(grid, lv_color_hex(0x000000), 0);

  // get rid of padding so cells can fill the entire screen
  lv_obj_set_style_pad_left(grid, 0, 0);
  lv_obj_set_style_pad_right(grid, 0, 0);
  lv_obj_set_style_pad_top(grid, 0, 0);
  lv_obj_set_style_pad_bottom(grid, 0, 0);

  // so we don't make a new variable each time in the loop
  lv_obj_t *valueLabel;
  lv_obj_t *cellLabel;
  lv_obj_t *cell;

  lv_subject_t *subscribers[4] = {&temperatureSubscriber, &humiditySubscriber, &lightingSubscriber, &soundSubscriber};
  // we want to borrow the value from the subscribers, which is why we have the & symbol
  // if we didn't do this we'd get a copy of the temperature subscriber (which then doesn't have the callback called every time, meaning we'd just
  // have one initial reading and then nothing)

  // yes it's a static value, could later make it dynamic by removing static from cellColumns and cellRows
  // but the documentation does not seem to like that, so for now it's kept
  for (int i = 0; i < SENSOR_AMOUNT; i++)
  {
    // get position of cell in the grid and create the cell object
    int column = i % 2;
    int row = i / 2;

    cell = lv_obj_create(grid);
    lv_obj_set_style_bg_color(cell, lv_color_hex(0x16213E), 0); // dark blue cell background
    lv_obj_set_style_border_color(cell, lv_color_hex(0x16213E), 0);

    // set the cell's position in the grid and have it stretch to fill the entire width & height of the cell
    lv_obj_set_grid_cell(cell, LV_GRID_ALIGN_STRETCH, column, 1, LV_GRID_ALIGN_STRETCH, row, 1);

    cellLabel = lv_label_create(cell);
    valueLabel = lv_label_create(cell);

    // last parameter as per docs is user_data which is of type void *, void * just means that you can use any type
    // however we then need to handle that in the callback when we use it (i.e. converting the void * to SensorMeta)
    // basically a generic if you know abt it
    lv_subject_add_observer_obj(subscribers[i], statusChangedCallback, valueLabel, (void *)&sensorMeta[i]);

    lv_label_set_text_fmt(cellLabel, "%s", cellLabels[i]);
    lv_obj_set_style_text_color(cellLabel, lv_color_hex(0xFFFFFF), 0); // white sensor name

    lv_obj_align(cellLabel, LV_ALIGN_TOP_MID, 0, 0); // align cell label to top mid without an offset
    lv_obj_center(valueLabel);
  }

  // reduce gap between cells
  lv_obj_set_style_pad_row(grid, 4, 0);
  lv_obj_set_style_pad_column(grid, 4, 0);
}


// maybe should be renamed as it now handles both values screen and status screen
void displayRegularValues() {
    statusScreen = lv_obj_create(NULL);
    valuesScreen = lv_obj_create(NULL);

    lv_obj_set_size(statusScreen, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_size(valuesScreen, LV_HOR_RES_MAX, LV_VER_RES_MAX);

    displayStatusGrid(statusScreen);
    display2x2Grid(valuesScreen);

    lv_screen_load(statusScreen); // start on status screen by default
}


// handling the wifi events (click unlocked wifi button or press enter key when done typing password)
static void wifiEventHandler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e); // event
  lv_obj_t *obj = lv_event_get_target_obj(e); // the object that triggered the event
  lv_obj_t *label = lv_obj_get_child(obj, 0); // first child of object (we know its a label i added it)

  int result;
  // if the event is button clicked
  if (code == LV_EVENT_CLICKED) {
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0); // set text color to white
    lv_display_refr_timer(NULL); // without this, text color will not change to white, more below

    result = connectToWifi(lv_label_get_text(label), "");

    if (result == -1) {
      lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0); // set text color to red
    }
    
  } else if (code == LV_EVENT_READY) { // if the event is text area being ready (enter on keyboard has been pressed)
    lv_obj_t *label = lv_obj_get_child(obj, 1); // second child because text area first child is a label for the text being inputted, obj is the textarea object
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0); // set text color to white
    lv_obj_set_style_border_color(obj, lv_color_hex(0x444444), 0);
    lv_display_refr_timer(NULL); // assumption, lvgl does not refresh the above text color in this event unless it deems it as an invalid area (which it does not)

    result = connectToWifi(lv_label_get_text(label), lv_textarea_get_text(obj)); // plug into dummy function

    if (result == -1) {
      lv_obj_set_style_text_color(label, lv_color_hex(0xFF0000), 0); // set text color to red
      lv_obj_set_style_border_color(obj, lv_color_hex(0xFF0000), 0);
    }
  }

  if (result == 1) {
    connectToMQTT();
    displayRegularValues();
  }
}


static void pageSwitch(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e); // event
  lv_obj_t *obj = lv_event_get_target_obj(e); // obj that triggered event

  // event_value_changed is triggered by menu upon change in the menu
  if (code == LV_EVENT_VALUE_CHANGED) { // that means either it loads the main or any of the sub pages
    lv_obj_t *page = lv_menu_get_cur_main_page(obj); // get current main page

    if (page == subPage) { // if its a subpage
      lv_obj_t *backButton = lv_menu_get_main_header_back_button(obj);
      lv_group_add_obj(buttons, backButton);
      lv_obj_remove_style(backButton, NULL, LV_STATE_FOCUS_KEY);
      lv_group_focus_obj(backButton); // focus the back btn so its clickable by the top button

    } else {
      lv_group_focus_obj(current); // focus back on the button that was clicked
    }
  }
}


void createWiFiMenu(int available, int wifi[]) {
    lv_obj_t *cont; // =container
    lv_obj_t *label; // =text
    lv_obj_t *btn; // =button
    lv_obj_t *menu = lv_menu_create(lv_screen_active()); // menu object

    // style it (size, color, centering)
    lv_obj_set_size(menu, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), 0);
    lv_obj_add_event_cb(menu, pageSwitch, LV_EVENT_VALUE_CHANGED, NULL);

    // make main page (list of wifis) and subpage (potential password input)
    mainPage = lv_menu_page_create(menu, NULL);
    subPage = lv_menu_page_create(menu, NULL); 

    // for every available wifi make a container with text that has [name of wifi] [indicator if it is locked or not]
    for (int i = 0; i < available; i++) {
      cont = lv_menu_cont_create(mainPage);
      btn = lv_button_create(cont);
      label = lv_label_create(btn);

      lv_obj_set_style_size(cont, LV_HOR_RES_MAX, 30, 0); // set container size
      
      if (WiFi.encryptionType(wifi[i]) != WIFI_AUTH_OPEN) {
        lv_menu_set_load_page_event(menu, btn, subPage); // if the button is clicked the passwordPage subpage is loaded
        lv_obj_add_event_cb(btn, loadPasswordPage, LV_EVENT_CLICKED, NULL); // run loadPasswordPage on button click

      } else {
        lv_obj_add_event_cb(btn, wifiEventHandler, LV_EVENT_CLICKED, NULL);
      }

      // make default button style
      static lv_style_t btnDefault;
      lv_style_init(&btnDefault);
      lv_style_set_size(&btnDefault, LV_HOR_RES_MAX, 30); // make it the entire of the screen horizontally and 30px high
      lv_style_set_shadow_width(&btnDefault, 0); // remove shadow
      lv_style_set_bg_opa(&btnDefault, LV_OPA_TRANSP); // bg transparent when not focused
      lv_style_set_radius(&btnDefault, 0); // remove border radius

      // make button style for when it is focused
      static lv_style_t btnFocused;
      lv_style_init(&btnFocused);
      lv_style_set_bg_opa(&btnFocused, LV_OPA_COVER); // full opacity bg when focused
      lv_style_set_bg_color(&btnFocused, lv_color_hex(0x333333)); // set bg to dark grey when focused

      // add styles to corresponding states
      lv_obj_add_style(btn, &btnDefault, LV_STATE_DEFAULT);
      lv_obj_add_style(btn, &btnFocused, LV_STATE_FOCUS_KEY);
      
      const char *locked = (WiFi.encryptionType(wifi[i]) == WIFI_AUTH_OPEN) ? "" : " (locked)"; // if true (wifi is auth open) string is empty else its "(locked)" 
      lv_label_set_text_fmt(label, "%s %s", WiFi.SSID(wifi[i]).c_str(), locked); // me when String and char[] are not the same :-(
      lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0); // set text color to white
  }

    lv_menu_set_page(menu, mainPage); // put that beautiful menu (billion containers) on the main page
}


void scan() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  int found = WiFi.scanNetworks();
  int availableWiFi[found];
  int wifiAmount = getAvailableWiFi(found, availableWiFi);

  createWiFiMenu(wifiAmount, availableWiFi);
}


// connect to wifi function
int connectToWifi(const char name[], const char password[]) {
  unsigned long startTime = millis();
  unsigned long previousTime = millis();
  unsigned long period = 15000; //ms

  while (WiFi.status() != WL_CONNECTED && startTime - previousTime <= period ) {
    if (password == "") {
      WiFi.begin(name);
    }
    else {
      WiFi.begin(name, password);
    }

    startTime = millis();
  }

  if (startTime - previousTime >= period) {
    return -1;
  }

  isConnectedToWiFi = 1;
  return 1;
}

void connectToMQTT() {
  unsigned long startTime = millis();
  unsigned long previousTime = millis();
  unsigned long period = 15000; //ms

  while (!client.connect(clientId) && startTime - previousTime <= period ) {
    delay(5000);
    startTime = millis();
    Serial.println("Waiting for MQTT to establish a connection.");
  }

  if (startTime - previousTime > period) {
    Serial.println("MQTT connection failed, device running locally. Restart if you'd like to retry.");
    return;
  }

  client.setCallback(mqttCallback); // callback for the app
  client.subscribe(appBoundsTopic); // subscribing for wio to receive values from app

  Serial.println("MQTT connection established.");
  isConnectedToMQTT = 1;
}


// display flushing function (= lvgl makes a graphic but it needs a function to write it to the screen so this is the function)
void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  // use tft to write [the chunk of the graphics LVGL rendered] to the screen
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}


static void loadPasswordPage(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target_obj(e);
  current = btn;

  char *label = lv_label_get_text(lv_obj_get_child(btn, 0)); // get the label of the button that triggered the event
  char wifiName[strlen(label)]; // make new wifiName string
  strcpy(wifiName, label); // copy label to it
  wifiName[strlen(label) - 10] = '\0'; // remove " (locked)" by adding string cutoff 

  static lv_obj_t *title = lv_label_create(subPage);
  lv_label_set_text_fmt(title, "Enter password for %s:", wifiName); // display that at the top
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); // white text
  lv_obj_set_style_margin_bottom(title, 8, 0);

  static lv_obj_t *passwordField = lv_textarea_create(subPage); // create password input field
  lv_textarea_set_password_mode(passwordField, true); // hide the actual password
  lv_textarea_set_one_line(passwordField, true); // one line input
  lv_obj_add_event_cb(passwordField, wifiEventHandler, LV_EVENT_READY, NULL);

  // make label with wifi name but hide it 
  static lv_obj_t *wifiLabel = lv_label_create(passwordField);
  lv_label_set_text(wifiLabel, wifiName);
  lv_obj_add_flag(wifiLabel, LV_OBJ_FLAG_HIDDEN);

  // style the field
  lv_obj_remove_style(passwordField, NULL, LV_STATE_FOCUS_KEY);
  lv_obj_set_width(passwordField, LV_HOR_RES_MAX - 20);
  lv_obj_set_style_margin_bottom(passwordField, 46, 0);
  lv_obj_set_style_bg_color(passwordField, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_color(passwordField, lv_color_hex(0x444444), 0);
  lv_obj_set_style_text_color(passwordField, lv_color_hex(0xFFFFFF), 0);

  static lv_obj_t *keyboard = lv_keyboard_create(subPage); // keyboard widget
  lv_keyboard_set_textarea(keyboard, passwordField); // link the keyboard to the password input field
  lv_group_focus_obj(keyboard); // focus kb
  
  // prettify the keyboard
  lv_obj_remove_style(keyboard, NULL, LV_STATE_FOCUS_KEY);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x333333), LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x444444), LV_PART_ITEMS | LV_STATE_FOCUSED);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) { // mqtt callback to parse painpatrol slider values
  if (strcmp(topic, appBoundsTopic) == 0) {
    JsonDocument doc;
    deserializeJson(doc, payload, length);
    sensorMeta[0].minVal = doc["bounds"]["temperature"]["min"];
    sensorMeta[0].maxVal = doc["bounds"]["temperature"]["max"];
    sensorMeta[1].minVal = doc["bounds"]["humidity"]["min"];
    sensorMeta[1].maxVal = doc["bounds"]["humidity"]["max"];
    sensorMeta[2].minVal = doc["bounds"]["lighting"]["min"];
    sensorMeta[2].maxVal = doc["bounds"]["lighting"]["max"];
    sensorMeta[3].minVal = doc["bounds"]["sound"]["min"];
    sensorMeta[3].maxVal = doc["bounds"]["sound"]["max"];
  }
}

static void returnCallback(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_CLICKED) {
    lv_screen_load(statusScreen);
  }
}


static void markCallback(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_CLICKED) {
    SensorComfort *sensorComfort = (SensorComfort *)lv_event_get_user_data(event);

    lv_subject_t *subscribers[4] = {&temperatureSubscriber, &humiditySubscriber, &lightingSubscriber, &soundSubscriber};

    // since subscriber and sensorComfort are both pointers, we can't really get their size since we just get the size of the pointer, so we just use sensor_amount
    for (int i = 0; i < SENSOR_AMOUNT; i++) {
      int comfort = uncomfortableValues[sensorComfort[i].messagesIndex];
      float currentValue = lv_subject_get_float(subscribers[i]); // sensor's value
      float boundChange = (i == 0) ? 1.0 : 5.0; // 1.0 for temperature, 5.0 for the rest of the sensors

      if (comfort == -1) {
        float lowerBound = currentValue + boundChange; // changes the lowerbound when user marks as too low
        if (lowerBound >= sensorMeta[i].maxVal) {
          Serial.print("Lower bound cannot exceed upper bound"); 
        } else {
          sensorMeta[i].minVal = lowerBound;
        }
      } else if ( comfort == 1) {
        float upperBound = currentValue - boundChange; // changes the upperBound when user marks the sensor as too high
        if (upperBound <= sensorMeta[i].minVal) {
          Serial.print("Upper bound cannot be less than lower bound");
        } else {
          sensorMeta[i].maxVal = upperBound;
        }
      }
    }
    if (isConnectedToMQTT == 1) {
      char boundsJSON[200];
      sprintf(boundsJSON, "{\"bounds\":{\"temperature\":{\"min\":%.2f,\"max\":%.2f},\"humidity\":{\"min\":%.2f,\"max\":%.2f},\"lighting\":{\"min\":%.2f,\"max\":%.2f},\"sound\":{\"min\":%.2f,\"max\":%.2f}}}",
        sensorMeta[0].minVal, sensorMeta[0].maxVal,
        sensorMeta[1].minVal, sensorMeta[1].maxVal,
        sensorMeta[2].minVal, sensorMeta[2].maxVal,
        sensorMeta[3].minVal, sensorMeta[3].maxVal);

      client.publish(wioBoundsTopic, boundsJSON); // publishes the new bounds to mqtt
    }

    lv_screen_load(statusScreen);
  }
}


static void changeMarkingCallback(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);

  if (code == LV_EVENT_CLICKED) {
    lv_obj_t *button = lv_event_get_target_obj(event);
    SensorComfort *sensorComfort = (SensorComfort *)lv_event_get_user_data(event);

    // make sure to wrap the index around in case that we are about to get out of bounds w/ the next button click
    // previously i tried to do this with **messages, which had pointers to temperatureMsgs etc, but then calculating the size of that became hell on earth, so easier solution was to just store the index directly in a struct alongside the array, which is what's being done here
    if (sensorComfort->messagesIndex > 1) {
      sensorComfort->messagesIndex = 0;
    } else {
      sensorComfort->messagesIndex++;
    }

    // lv_obj_get_child of button gives label since button in this context only has label as a child
    lv_label_set_text_fmt(lv_obj_get_child(button, 0), "%s: %s", sensorComfort->sensorName, sensorComfort->messages[sensorComfort->messagesIndex]);
  }
}


// double pointers can look scary, but it's because initially we have a pointer to lv_obj_t for container, button and label, and since we now want to also use these in this function for reusability, we have to have another pointer to be able to get to it, then when we pass in the value, we borrow it (&) first
static void createContainer(const lv_style_t* defaultStyle, const lv_style_t* focusedStyle, lv_obj_t **container, lv_obj_t **button, lv_obj_t **label, char *text, int32_t padding) { // could maybe make styles global?
  *container = lv_menu_cont_create(mainPage);
  *button = lv_button_create(*container);
  *label = lv_label_create(*button);

  lv_obj_set_size(*container, LV_HOR_RES_MAX, 30);

  lv_obj_center(*container);

  lv_obj_set_style_pad_bottom(*container, padding, 0); // add spacing between containers
    
  lv_label_set_text(*label, text);
  lv_obj_set_style_text_color(*label, lv_color_hex(0xFFFFFF), 0);

    // add styles to corresponding states
  lv_obj_add_style(*button, defaultStyle, LV_STATE_DEFAULT);
  lv_obj_add_style(*button, focusedStyle, LV_STATE_FOCUS_KEY);
}


static void displayUncomfortableSelectionMenu() {
  lv_group_remove_all_objs(mainGroup);
  uncomfortableSelectionScreen = lv_obj_create(NULL);
  lv_obj_set_size(uncomfortableSelectionScreen, LV_HOR_RES_MAX, LV_VER_RES_MAX);

  lv_screen_load(uncomfortableSelectionScreen);

  lv_obj_t *menu = lv_menu_create(lv_screen_active()); // create menu

  lv_obj_set_size(menu, LV_HOR_RES_MAX, LV_VER_RES_MAX);
  lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), 0);
  lv_obj_center(menu); // style menu

  mainPage = lv_menu_page_create(menu, NULL); // load page

  lv_obj_t *header = lv_label_create(mainPage);
  lv_label_set_text(header, "Mark Uncomfortable Values");
  lv_obj_set_style_pad_top(header, 10, 0);
  lv_obj_set_style_pad_bottom(header, 10, 0);
  lv_obj_set_style_text_color(header, lv_color_hex(0xFFFFFF), 0);

  char textBuffer[25];
  const char *unit;
  lv_obj_t *container;
  lv_obj_t *button;
  lv_obj_t *label;

  // make default button style
  static lv_style_t buttonDefault;
  lv_style_init(&buttonDefault);
  lv_style_set_size(&buttonDefault, LV_HOR_RES_MAX, 30);
  lv_style_set_shadow_width(&buttonDefault, 0); // remove shadow
  lv_style_set_bg_opa(&buttonDefault, LV_OPA_TRANSP); // bg transparent when not focused
  lv_style_set_radius(&buttonDefault, 0); // remove border radius

  // make button style for when it is focused
  static lv_style_t buttonFocused;
  lv_style_init(&buttonFocused);
  lv_style_set_size(&buttonFocused, LV_HOR_RES_MAX, 30);
  lv_style_set_bg_opa(&buttonFocused, LV_OPA_COVER); // full opacity bg when focused
  lv_style_set_bg_color(&buttonFocused, lv_color_hex(0x333333)); // set bg to dark grey when focused

  static SensorComfort sensorComfort[] = {
    {"Temperature", temperatureMsgs, 0},
    {"Humidity", humidityMsgs, 0},
    {"Lighting", lightingMsgs, 0},
    {"Sound", soundMsgs, 0}
  };

  // since sensorComfort is an array of SensorComforts, we divide by the size of the SensorComfort type
  for (int i = 0; i < SENSOR_AMOUNT; i++) {
    strcpy(textBuffer, ""); // clear string
    strcat(textBuffer, sensorComfort[i].sensorName);
    strcat(textBuffer, ": ");
    strcat(textBuffer, sensorComfort[i].messages[0]); // in the end will be e.g. Temperature: Good

    createContainer(&buttonDefault, &buttonFocused, &container, &button, &label, textBuffer, 5); // padding is 5

    // we want to borrow the sensorComfort struct, not create a copy of it
    lv_obj_add_event_cb(button, changeMarkingCallback, LV_EVENT_CLICKED, &sensorComfort[i]);
  }

  // add extra spacing on bottom to separate the labels with the buttons
  lv_obj_set_style_pad_bottom(container, 10, 0); // add spacing between containers

  createContainer(&buttonDefault, &buttonFocused, &container, &button, &label, "Mark", 5);

  // style the border separating the labels and buttons

  lv_obj_set_style_border_side(container, (lv_border_side_t)LV_BORDER_SIDE_TOP, 0);

  lv_obj_set_style_border_width(container, 1, 0);

  lv_obj_set_style_border_color(container, lv_color_hex(0xFFFFFF), 0);

  lv_obj_set_style_border_opa(container, LV_OPA_70, 0);

  // call Mark back /joke please don't look at this when grading
  lv_obj_add_event_cb(button, markCallback, LV_EVENT_CLICKED, sensorComfort);

  createContainer(&buttonDefault, &buttonFocused, &container, &button, &label, "Return", 5);

  lv_obj_add_event_cb(button, returnCallback, LV_EVENT_CLICKED, NULL);

  lv_menu_set_page(menu, mainPage); // display the page
}


// just initializing the display as per lvgl docs
void createDisplay() {
  lv_display_t *disp = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX); // create display instance
  lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL); // set the buffer for the display
  lv_display_set_flush_cb(disp, displayFlush); // choose flushing funct
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED); // bit swap colors because wio is little endian
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0); // set bg color because default for lvgl is white (yuck)
}


// check if wifiName is already in the unique wifi array
bool wifiInArray(int unique[], String wifiName, int uniqueAmount) {
  for (int i = 0; i < uniqueAmount; i++) {
    if (wifiName == WiFi.SSID(unique[i])) {
      return true;
    }
  }

  return false;
}


// write unique wifi indices into array, return amount of unique wifis found
int getAvailableWiFi(int wifiAmount, int unique[]) {
  int counter = 0; // counter for unique wifi amount

  if (wifiAmount == 0) {
    Serial.println("No networks found :-(");

  } else {
    Serial.println("Available networks:");
    for (int i = 0; i < wifiAmount; i++) {
      if (!wifiInArray(unique, WiFi.SSID(i), counter) && WiFi.SSID(i) != "") { // if the wifi is not in the unique wifi array already and the name is not empty
        unique[counter] = i; // put the index of the unique wifi into the array of unique wifis
        counter++; // update the amount of unique wifi
        
        // print them in terminal just to check
        Serial.print(counter);
        Serial.print(": ");
        Serial.print(WiFi.SSID(i));
        Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : " (locked)");
      }
    }
  }

  return counter;
}


// navigating between colour-coded page and values page
// moving the joystick to the right takes you to the actual values
void handleScreenSwitch(lv_indev_data_t *data)
{
  lv_obj_t *current = lv_screen_active(); // could probably rework the logic of also having current == uncomfortableSelectionScreen, maybe if we'd have more menus we could just ensure that current != wifiHandler page

  if (data->key == LV_KEY_RIGHT && (current == statusScreen || current == uncomfortableSelectionScreen)) {
    lv_screen_load(valuesScreen); // right: values screen
  } else if (data->key == LV_KEY_LEFT && (current == valuesScreen || current == uncomfortableSelectionScreen)) {
    lv_screen_load(statusScreen); // left: back to status screen
  }
}


// function for reading 5 way switch as per lvgl docs template for any input device
void readSwitch(lv_indev_t *indev, lv_indev_data_t *data) {
  data->key = 0; // assign nonexistent key
  data->state = LV_INDEV_STATE_PRESSED; // so we can make it's initial state pressed so we dont have to repeatedly set the state to pressed in the first 5 cases #lazy
  lv_obj_t *focused = lv_group_get_focused(mainGroup); // get the focused element

  // when the funct is called and the 5 way switch is activated in some way set the key to the corresponding lvgl key
  // so 5 way switch right is read as lv_key_right etc.
  if (digitalRead(WIO_5S_UP) == LOW) {
    data->key = (lv_obj_check_type(focused, &lv_keyboard_class)) ? LV_KEY_UP : LV_KEY_PREV; // if focused is of class keyboard then its up if not its prev
  
  } else if (digitalRead(WIO_5S_DOWN) == LOW) {
    data->key = (lv_obj_check_type(focused, &lv_keyboard_class)) ? LV_KEY_DOWN : LV_KEY_NEXT;  // sorry guys i started liking the ternary operator too much
  
  } else if (digitalRead(WIO_5S_LEFT) == LOW) {
    data->key = LV_KEY_LEFT;
    
  } else if (digitalRead(WIO_5S_RIGHT) == LOW) {
    data->key = LV_KEY_RIGHT;

  } else if (digitalRead(WIO_5S_PRESS) == LOW) {
    data->key = LV_KEY_ENTER;

  } else { // if it doesn't read anything from the 5 way switch make the key state released
    data->state = LV_INDEV_STATE_RELEASED;
  }

  // maybe there is a better place to put it but it works for now (?)
  handleScreenSwitch(data); // handle screen switching (moving right/left between pages)
}   


void setupSwitch(lv_indev_t *wioSwitch) {
  // setup pins as input
  pinMode(WIO_5S_UP, INPUT_PULLUP);
  pinMode(WIO_5S_DOWN, INPUT_PULLUP);
  pinMode(WIO_5S_LEFT, INPUT_PULLUP);
  pinMode(WIO_5S_RIGHT, INPUT_PULLUP);
  pinMode(WIO_5S_PRESS, INPUT_PULLUP);

  lv_indev_set_type(wioSwitch, LV_INDEV_TYPE_KEYPAD); // set device type to keypad (no other input type fits /shrug)
  lv_indev_set_read_cb(wioSwitch, readSwitch); // set the input reading function
}


// same as switch
void readButton(lv_indev_t *indev, lv_indev_data_t *data) {
  data->state = LV_INDEV_STATE_PRESSED; 

  // WHY IS EVERYTHING BACKWARDS ON THIS DEVICE I SPENT AN HOUR THINKING THERE WAS SOMETHIGN WRONG WITH MY LOGIC
  // and i ended up removing most of it because the coordinate system is so obscure.. this is wrong but easier to manage
  if (digitalRead(WIO_KEY_C) == LOW) { // C is top left button youre welcome. if you add any of the other buttons into this handler
    // then they will all be valid presses on the back button so i think if we need the other buttons we will need to make separate handlers
    // or we dont use data->key but call events instead
    // or we use data->key only for wio_key_c and have the other buttons serve diff purposes
    data->key = LV_KEY_ENTER;
  
  } 
  else if (digitalRead(WIO_KEY_B) == LOW) {
    if (lv_screen_active() == statusScreen || lv_screen_active() == valuesScreen) { // only allow switching to uncomfortable values if not in wifi screen and uncomfortable values screen
      displayUncomfortableSelectionMenu();
    }
    
  }
  else if (digitalRead(WIO_KEY_A) == LOW) {
    toggleBuzzerMute();
  }
  else {
    data->state = LV_INDEV_STATE_RELEASED; 

  }
}


void setupButton(lv_indev_t *wioButton) {
  pinMode(WIO_KEY_A, INPUT_PULLUP);
  pinMode(WIO_KEY_B, INPUT_PULLUP);
  pinMode(WIO_KEY_C, INPUT_PULLUP);

  lv_indev_set_type(wioButton, LV_INDEV_TYPE_KEYPAD); // blasphemy
  lv_indev_set_read_cb(wioButton, readButton); // set the input reading function
}


// basic tft text display funct for before lvgl is done making first page
void displayText(char text[]) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(text, LV_HOR_RES_MAX / 4 + 9, LV_VER_RES_MAX / 2 - 10);
}

// function to calculate the average for all the readings in the buffer
float calculateAverage(float* buffer, int count) { 
  float sum = 0;
  for (int i = 0; i < count; i++) {
    if (buffer[i] != -50) {
      sum += buffer[i];
    }
    else {
      count--;
    }
  }
  return sum / count;
}

void setup() {
  Serial.begin(115200); // begin terminal
  rtc.begin();

  DateTime now = DateTime(F(__DATE__), F(__TIME__)); // provides current date and time during compilation
  now = now - TimeSpan(TIMEZONE_OFFSET); // datetime is set to utc
  rtc.adjust(now); // adjusts the RTC clock so that it can give accurate epochs

  isConnectedToMQTT = 0;
  isConnectedToWiFi = 0;
  isBuzzerMuted = 0;
  lv_subject_init_float(&temperatureSubscriber, 0);
  lv_subject_init_float(&humiditySubscriber, 0);
  lv_subject_init_float(&soundSubscriber, 0);
  lv_subject_init_float(&lightingSubscriber, 0);

  dht.begin();

  client.setServer(mqttHost, port);

  tft.begin(); // start tft
  tft.setRotation(3); // set rotation to 180 (wio 0 is upside down if you want the 5way switch at the bottom)

  lv_init(); // initialize lvgl
  lv_tick_set_cb(tick); // set tick func

  createDisplay();
  displayText("Scanning for networks..."); // loading screen text :3

  // allocate built-in buzzer to be output
  pinMode(WIO_BUZZER, OUTPUT);
  globalBuzzerTime = 0;

  lv_indev_t *wioSwitch = lv_indev_create(); // create input device instance for 5 way switch
  lv_indev_t *wioButton = lv_indev_create(); // create for button
  setupSwitch(wioSwitch); // set up
  setupButton(wioButton); // yes

  mainGroup = lv_group_create(); // create group for objects
  buttons = lv_group_create(); // create group for buttons
  lv_indev_set_group(wioSwitch, mainGroup); // make 5 way switch the input device for this group of objects
  lv_indev_set_group(wioButton, buttons);
  lv_group_set_default(mainGroup); // set it as the default group for all objects created after this point 

  scan();
}

void loop() {
  lv_timer_handler(); 
  delay(5);

  if (isConnectedToWiFi == 0) {
    return;
  }

  static float tempBuffer[100], humidBuffer[100], lightBuffer[100],  soundBuffer[100];
  static int bufferIndex = 0;

  float temperatureHumidityValues[2] = {0};
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float lightValue = analogRead(LIGHT_PIN) / 10.0; // get percentage since value is in 1000s
  float soundValue = analogRead(SOUND_PIN) / 10.0;


  if (dht.readTempAndHumidity(temperatureHumidityValues)) { // if readTempAndHumidity returns 1, it's an error, don't ask me idk why they'd have it like this
      Serial.println("Failed to get temperature and humidity value.");
      temperatureHumidityValues[0] = -50.0; // indicate failure that gets sent to mqtt, ignore -50 in avg calc
      temperatureHumidityValues[1] = -50.0 ;
  }

  // code for receiving sensor values above, every time a value is received, append it to a list (assuming ino can work with lists)
  // calculateAverage(); calculate avg of every array reading and save it as a float
  // then update it in the subscriber for lvgl and mqtt

  if (bufferIndex < 100) { // puts values in buffer for avg function
    tempBuffer[bufferIndex] = temperatureHumidityValues[1];
    humidBuffer[bufferIndex] = temperatureHumidityValues[0];
    soundBuffer[bufferIndex] = soundValue;
    lightBuffer[bufferIndex] = lightValue;
    bufferIndex++;
  } 
  
  if (isConnectedToMQTT == 1) {
    client.loop();
  }

  if (millis() - publishTime >= 10000) { // sends readings every 10 seconds
    publishTime = millis();
    lv_subject_set_float(&temperatureSubscriber, calculateAverage(tempBuffer, bufferIndex)); // calculate average func calls for all readings
    lv_subject_set_float(&humiditySubscriber, calculateAverage(humidBuffer, bufferIndex));
    lv_subject_set_float(&soundSubscriber, calculateAverage(soundBuffer, bufferIndex));
    lv_subject_set_float(&lightingSubscriber, calculateAverage(lightBuffer, bufferIndex));
  
    
    if (isConnectedToMQTT == 1) {
        // MQTT gets called here

      char readingsJSON[200]; // buffer for JSON
      sprintf(readingsJSON, "{\"timestamp\":%ld,\"readings\":{\"temperature\":%.2f,\"humidity\":%.2f,\"lighting\":%.2f,\"sound\":%.2f}}",
        rtc.now().unixtime(), // unix time epoch
        temperatureHumidityValues[1],
        temperatureHumidityValues[0],
        lightValue,
        soundValue);

      client.publish(readingsTopic, readingsJSON);
    }
    bufferIndex = 0;
  }
}