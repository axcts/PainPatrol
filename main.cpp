#include <Arduino.h>
#include <lvgl.h>     // graphics library
#include <TFT_eSPI.h> // (library for LCD display)
#include "rpcWiFi.h"  // wifi library
#include "conf.h"     // add custom constants

TFT_eSPI tft = TFT_eSPI();                  // tft instance
static lv_color_t buf[LV_HOR_RES_MAX * 10]; // display buffer for LVGL (defining how big of a chunk of the display LVGL can work on at once)

// display flushing function (= lvgl makes a graphic but it needs a function to write it to the screen so this is the function)
void displayFlush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    // use tft to write [the chunk of the graphics LVGL rendered] to the screen
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

void createWiFiMenu(int available, int wifi[])
{
    lv_obj_t *cont;                                      // =container
    lv_obj_t *label;                                     // =text
    lv_obj_t *menu = lv_menu_create(lv_screen_active()); // menu object

    // style it (size, color, centering)
    lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
    lv_obj_set_style_bg_color(menu, lv_color_hex(0x000000), 0);
    lv_obj_center(menu);

    // make main page (list of wifis) and later add subpage (potential password input)
    lv_obj_t *main_page = lv_menu_page_create(menu, NULL);

    // for every available wifi make a container with text that has [name of wifi] [indicator if it is locked or not]
    for (int i = 0; i < available; i++)
    {
        cont = lv_menu_cont_create(main_page);
        label = lv_label_create(cont);
        const char *locked = (WiFi.encryptionType(wifi[i]) == WIFI_AUTH_OPEN) ? "" : " (locked)"; // if true (wifi is auth open) string is empty else its "(locked)"
        lv_label_set_text_fmt(label, "%s %s", WiFi.SSID(wifi[i]).c_str(), locked);                // me when String and char[] are not the same :-(
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);                            // set text color to white
    }

    lv_menu_set_page(menu, main_page); // put that beautiful menu (billion containers) on the main page
}

// just initializig the display as per lvgl docs
void createDisplay()
{
    lv_display_t *disp = lv_display_create(LV_HOR_RES_MAX, LV_VER_RES_MAX);               // create display
    lv_display_set_buffers(disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL); // set the buffer for the display
    lv_display_set_flush_cb(disp, displayFlush);                                          // choose flushing funct
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);                    // bit swap colors because wio is little endian
    lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x000000), 0);             // set bg color because default for lvgl is white (yuck)
}

// check if wifiName is already in the unique wifi array
bool wifiInArray(int unique[], String wifiName, int uniqueAmount)
{
    for (int i = 0; i < uniqueAmount; i++)
    {
        if (wifiName == WiFi.SSID(unique[i]))
        {
            return true;
        }
    }

    return false;
}

// write unique wifi indices into array, return amount of unique wifis found
int getAvailableWiFi(int wifiAmount, int unique[])
{
    int counter = 0; // counter for unique wifi amount

    if (wifiAmount == 0)
    {
        Serial.println("No networks found :-(");
    }
    else
    {
        Serial.println("Available networks:");
        for (int i = 0; i < wifiAmount; i++)
        {
            if (!wifiInArray(unique, WiFi.SSID(i), counter) && WiFi.SSID(i) != "")
            {                        // if the wifi is not in the unique wifi array already and the name is not empty
                unique[counter] = i; // put the index of the unique wifi into the array of unique wifis
                counter++;           // update the amount of unique wifi

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

void setup()
{
    Serial.begin(115200); // begin terminal

    tft.begin();        // start tft
    tft.setRotation(3); // set rotation to 180 (wio 0 is upside down if you want the 5way switch at the bottom)

    lv_init(); // initialize lvgl
    createDisplay();

    int found = WiFi.scanNetworks();
    int availableWiFi[found];
    int wifiAmount = getAvailableWiFi(found, availableWiFi);

    createWiFiMenu(wifiAmount, availableWiFi);
}

void loop()
{
    lv_timer_handler();
    delay(5);
}
