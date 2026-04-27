#include <TFT_eSPI.h> // library for LCD display
#include "rpcWiFi.h"  // wifi lib

TFT_eSPI tft = TFT_eSPI(); // tft instance

void displayAvailableWifi()
{
    int found = WiFi.scanNetworks(); // get number of networks found
    int y = 0;                       // y coordinate

    if (found == 0)
    {
        tft.drawString("No networks found :-(", 0, 0);
    }
    else
    {
        for (int i = 0; i < found; i++)
        {
            if (WiFi.SSID(i) != "")
            {                                                                                       // check if the wifi has a name (sometimes it just hallucinates)
                Serial.println(WiFi.SSID(i));                                                       // print wifis in terminal just to check
                const char *locked = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : " (locked)"; // if wifi is auth open the expression is true and the string is empty if not then the string is "(locked)"

                tft.drawString(WiFi.SSID(i) + locked, 0, y); // draws string from position (x, y)
                y += 10;                                     // update y coord to draw next string 10 pixels below
            }
        }
    }
}

void setup()
{
    Serial.begin(115200); // begin terminal

    tft.begin();               // start tft
    tft.setRotation(3);        // set rotation to 180 (0 is upside down on wio unfortunately)
    tft.fillScreen(TFT_BLACK); // make bg black
    displayAvailableWifi();
}

void loop()
{
}