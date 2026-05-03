#include <lvgl.h> // graphics library
#include <TFT_eSPI.h> // (library for LCD display)
#include <PubSubClient.h> // mqtt
#include "DateTime.h"
#include "RTC_SAMD51.h" // RTC clock so that our microcontroller knows when 1970 was compared to now
#include "rpcWiFi.h" // wifi library
#include "conf.h" // custom constants
#include "Grove_Temperature_And_Humidity_Sensor.h"

DHT dht(DHT_PIN, DHT_TYPE);
RTC_SAMD51 rtc; // https://wiki.seeedstudio.com/Wio-Terminal-RTC/

WiFiClient wifiClient;
PubSubClient client(wifiClient); // create wifi client for mqtt

const char * mqttHost = "broker.hivemq.com";
const int port = 1883;
const char * readingsTopic = "painpatrol/readings"; // for publishing
const char * clientId = "wio-terminal";

TFT_eSPI tft = TFT_eSPI(); // tft instance
static lv_color_t buf[LV_HOR_RES_MAX * 10]; // display buffer for LVGL (defining how big of a chunk of the display LVGL can work on at once)
static uint32_t tick(void) { return millis(); } // tick func

// Subscribers inside of LVGL, not MQTT!!
lv_subject_t temperatureSubscriber;
lv_subject_t humiditySubscriber;
lv_subject_t soundSubscriber;
lv_subject_t lightingSubscriber;

int isConnectedToWiFi;
int isConnectedToMQTT;

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

    const char *cellLabels[] = {"Temperature", "Humidity", "Lighting", "Sound"};
    lv_subject_t *subscribers[4] = {&temperatureSubscriber, &humiditySubscriber, &lightingSubscriber, &soundSubscriber};
    // we want to borrow the value from the subscribers, which is why we have the & symbol
    // if we didn't do this we'd get a copy of the temperature subscriber (which then doesn't have the callback called every time, meaning we'd just
    // have one initial reading and then nothing)

    // yes it's a static value, could later make it dynamic by removing static from cellColumns and cellRows
    // but the documentation does not seem to like that, so for now it's kept
    for (int i = 0; i < 4; i++) {
        // get position of cell in the grid and create the cell object
        int column = i % 2;
        int row = i / 2;

        cell = lv_obj_create(grid);

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

        lv_obj_align(cellLabel, LV_ALIGN_TOP_MID, 0, 0); // align cell label to top mid without an offset
        lv_obj_center(valueLabel);

    }

    // reduce gap between cells
    lv_obj_set_style_pad_row(grid, 4, 0);
    lv_obj_set_style_pad_column(grid, 4, 0);
}

void displayRegularValues() {
    // create an empty screen to then switch to and set its size
    lv_obj_t *valuesScreen = lv_obj_create(NULL);
    lv_screen_load(valuesScreen);
    lv_obj_set_size(valuesScreen, LV_HOR_RES_MAX, LV_VER_RES_MAX);

    display2x2Grid(valuesScreen);
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

void createWiFiMenu(int available, int wifi[]) {
    lv_obj_t *cont; // =container
    lv_obj_t *label; // =text
    lv_obj_t *btn; // =button
    lv_obj_t *menu = lv_menu_create(lv_screen_active()); // menu object

    // style it (size, color, centering)
    lv_obj_set_size(menu, LV_HOR_RES_MAX, LV_VER_RES_MAX);
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), 0);

    // make main page (list of wifis) and subpage (potential password input)
    
    lv_obj_t *wifiPage = lv_menu_page_create(menu, NULL);

    // for every available wifi make a container with text that has [name of wifi] [indicator if it is locked or not]
    for (int i = 0; i < available; i++) {
      cont = lv_menu_cont_create(wifiPage);
      btn = lv_button_create(cont);
      label = lv_label_create(btn);

      lv_obj_set_style_size(cont, LV_HOR_RES_MAX, 30, 0); // set container size
      
      if (WiFi.encryptionType(wifi[i]) != WIFI_AUTH_OPEN) {
        lv_obj_t *passwordPage = lv_menu_page_create(menu, NULL); // make unique password page for each encrypted wifi so we can connect the wifi name to it
        createPasswordPage(passwordPage, WiFi.SSID(wifi[i]).c_str()); // i know this is unnecessarily complicated but i dont know how else to go about it
        lv_menu_set_load_page_event(menu, btn, passwordPage); // if the button is clicked the passwordPage subpage is loaded (for locked wifis)

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

    lv_menu_set_page(menu, wifiPage); // put that beautiful menu (billion containers) on the main page
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

void createPasswordPage(lv_obj_t *page, const char wifiName[]) {
  lv_obj_t *title = lv_label_create(page);
  lv_label_set_text_fmt(title, "Enter password for %s:", wifiName); // display that at the top
  lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0); // white text
  lv_obj_set_style_margin_bottom(title, 8, 0);

  lv_obj_t *passwordField = lv_textarea_create(page); // create password input field
  lv_textarea_set_password_mode(passwordField, true); // hide the actual password
  lv_textarea_set_one_line(passwordField, true); // one line input
  lv_obj_add_event_cb(passwordField, wifiEventHandler, LV_EVENT_READY, NULL);

  // make label with wifi name but hide it
  lv_obj_t *wifiLabel = lv_label_create(passwordField);
  lv_label_set_text(wifiLabel, wifiName);
  lv_obj_add_flag(wifiLabel, LV_OBJ_FLAG_HIDDEN);
  

  // style it
  lv_obj_remove_style(passwordField, NULL, LV_STATE_FOCUS_KEY);
  lv_obj_set_width(passwordField, LV_HOR_RES_MAX - 20);
  lv_obj_set_style_margin_bottom(passwordField, 46, 0);
  lv_obj_set_style_bg_color(passwordField, lv_color_hex(0x333333), 0);
  lv_obj_set_style_border_color(passwordField, lv_color_hex(0x444444), 0);
  lv_obj_set_style_text_color(passwordField, lv_color_hex(0xFFFFFF), 0);

  lv_obj_t *keyboard = lv_keyboard_create(page); // keyboard widget
  lv_keyboard_set_textarea(keyboard, passwordField); // link the keyboard to the password input field
  
  // prettify the keyboard
  lv_obj_remove_style(keyboard, NULL, LV_STATE_FOCUS_KEY);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x333333), LV_PART_MAIN);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x333333), LV_PART_ITEMS);
  lv_obj_set_style_text_color(keyboard, lv_color_hex(0xFFFFFF), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x444444), LV_PART_ITEMS | LV_STATE_FOCUSED);
}


// just initializig the display as per lvgl docs
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


// function for reading 5 way switch as per lvgl docs template for any input device
void readSwitch(lv_indev_t *indev, lv_indev_data_t *data) {
  data->key = 0; // assign nonexistent key
  data->state = LV_INDEV_STATE_PRESSED; // so we can make it's initial state pressed so we dont have to repeatedly set the state to pressed in the first 5 cases #lazy


  // when the funct is called and the 5 way switch is activated in some way set the key to the corresponding lvgl key
  // so 5 way switch right is read as lv_key_right etc.
  if (digitalRead(WIO_5S_UP) == LOW) {
    data->key = LV_KEY_PREV;
  
  } else if (digitalRead(WIO_5S_DOWN) == LOW) {
    data->key = LV_KEY_NEXT;
  
  } else if (digitalRead(WIO_5S_LEFT) == LOW) {
    data->key = LV_KEY_LEFT;
    
  } else if (digitalRead(WIO_5S_RIGHT) == LOW) {
    data->key = LV_KEY_RIGHT;

  } else if (digitalRead(WIO_5S_PRESS) == LOW) {
    data->key = LV_KEY_ENTER;

  } else { // if it doesn't read anything from the 5 way switch make the key state released
    data->state = LV_INDEV_STATE_RELEASED;
  }
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

void displayText(char text[]) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString(text, LV_HOR_RES_MAX / 4 + 9, LV_VER_RES_MAX / 2 - 10);
}


void setup() {
  Serial.begin(115200); // begin terminal
  rtc.begin();

  DateTime now = DateTime(F(__DATE__), F(__TIME__)); // provides current date and time during compilation
  now = now - TimeSpan(TIMEZONE_OFFSET); // datetime is set to utc
  rtc.adjust(now); // adjusts the RTC clock so that it can give accurate epochs

  isConnectedToMQTT = 0;
  isConnectedToWiFi = 0;
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

  lv_indev_t *wioSwitch = lv_indev_create(); // create input device instance for 5 way switch
  setupSwitch(wioSwitch); // set it Up!

  static lv_group_t *wifiList = lv_group_create(); // create group for objects
  lv_indev_set_group(wioSwitch, wifiList); // make 5 way switch the input device for this group of objects
  lv_group_set_default(wifiList); // set it as the default group for all objects created after this point 
  // that is subject to change as we will have subpages and different input handling will be needed but it is ok for now

  scan();
}

void loop() {
  lv_timer_handler(); 
  delay(5);

  if (isConnectedToWiFi == 0) {
    return;
  }

  float temperatureHumidityValues[2] = {0};
    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  
  float lightValue = analogRead(LIGHT_PIN) / 10.0; // get percentage since value is in 1000s
  float soundValue = analogRead(SOUND_PIN) / 10.0;


  if (dht.readTempAndHumidity(temperatureHumidityValues)) { // if readTempAndHumidity returns 1, it's an error, don't ask me idk why they'd have it like this
      Serial.println("Failed to get temprature and humidity value.");
      temperatureHumidityValues[0] = -50.0; // indicate failure that gets sent to mqtt, ignore -50 in avg calc
      temperatureHumidityValues[1] = -50.0 ;
  }

  // code for receiving sensor values above, every time a value is received, append it to a list (assuming ino can work with lists)


  // calculateAverage(); calculate avg of every array reading and save it as a float
  // then update it in the subscriber for lvgl and mqtt
  lv_subject_set_float(&temperatureSubscriber, temperatureHumidityValues[1]);
  lv_subject_set_float(&humiditySubscriber, temperatureHumidityValues[0]);
  lv_subject_set_float(&soundSubscriber, soundValue);
  lv_subject_set_float(&lightingSubscriber, lightValue);

  if (isConnectedToMQTT == 0) { // clear lists here in case MQTT is off
    return;
  }

  

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
