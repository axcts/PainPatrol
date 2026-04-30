#include <lvgl.h> // graphics library
#include <TFT_eSPI.h> // (library for LCD display)
#include "rpcWiFi.h" // wifi library
#include "conf.h" // custom constants


TFT_eSPI tft = TFT_eSPI(); // tft instance
static lv_color_t buf[LV_HOR_RES_MAX * 10]; // display buffer for LVGL (defining how big of a chunk of the display LVGL can work on at once)
static uint32_t tick(void) { return millis(); } // tick func

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
    displayRegularValues();
  }
}

void displayRegularValues() {
    lv_obj_t *valuesScreen = lv_obj_create(NULL);
    lv_screen_load(valuesScreen);
    lv_obj_set_size(valuesScreen, LV_HOR_RES_MAX, LV_VER_RES_MAX);
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
      Serial.print(millis());
    }
    else {
      WiFi.begin(name, password);
      Serial.print(millis());
    }

    startTime = millis();
    Serial.print("ddddddd");
    Serial.print(startTime - previousTime);
  }

  if (startTime - previousTime >= period) {
    return -1;
  }

  return 1;
  
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
    Serial.print("pressed");

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
}
