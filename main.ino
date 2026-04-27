#include <TFT_eSPI.h> // library for LCD display
#include "rpcWiFi.h" // wifi lib

TFT_eSPI tft = TFT_eSPI(); // tft instance

void displayAvailableWifi(int wifiAmount, int availableWiFi[]) {
  int y = 0; // coordinate

  if (wifiAmount == 0) {
    tft.drawString("No networks found :-(", 0, 0);

  } else {
    for (int i = 0; i < wifiAmount; i++) {
      const char *locked = (WiFi.encryptionType(availableWiFi[i]) == WIFI_AUTH_OPEN) ? "" : " (locked)"; // if wifi is auth open the expression is true and the string is empty if not then the string is "(locked)"

      tft.drawString(WiFi.SSID(availableWiFi[i]) + locked, 0, y); // draws string from position (x, y)
      y += 10; // update y coord to draw next string 10 pixels below
    }
  }
}

// take in the unique wifi array, name of the wifi to check and how many elements to check in that array
// only check the first uniqueAmount elements as the unique wifi array has allocated enough space to fit all the wifis found
// but we should only check the amount of wifis we have already put into the array because the rest of the spaces will be empty anyway
bool wifiInArray(int unique[], String wifiName, int uniqueAmount) {
  for (int i = 0; i < uniqueAmount; i++) {
    if (wifiName == WiFi.SSID(unique[i])) {
      return true;
    }
  }

  return false;
}

// take in the total amount of wifi found and the array to write unique wifi indices into, return the amount of unique wifi
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

void setup() {
  Serial.begin(115200); // begin terminal
  
  int found = WiFi.scanNetworks(); // get number of networks found
  int availableWiFi[found]; // make array to store unique wifis
  int wifiAmount = getAvailableWiFi(found, availableWiFi); // get amount of unique wifis (the function also stores them in the wifi array)

  tft.begin(); // start tft
  tft.setRotation(3); // set rotation to 180 (0 is upside down on wio unfortunately)
  tft.fillScreen(TFT_BLACK); // make bg black
  
  displayAvailableWifi(wifiAmount, availableWiFi);
}

void loop() {
}
