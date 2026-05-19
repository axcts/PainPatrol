# Pain Patrol
## Overview
A system for chronic migraine sufferers which tracks light, sound, temperature, and humidity conditions and provides an evaluation of these conditions to aid the user in avoiding migraine triggers. The system allows customisation of comfortable environmental value ranges, and alerts the user when the readings are outside of the comfortable range. The mobile application allows the user to view graphs of environmental readings from a customisable timeframe.
### Demo (link goes here)

## Getting Started
### Prerequisites
- Computer
- Wio Terminal with sensors listed in [Hardware Specification](#hardware-specification)
- [Wio Terminal Chassis Battery](https://wiki.seeedstudio.com/Wio-Terminal-Chassis-Battery_650mAh/)
- [Git](https://git-scm.com/)
- [Android Studio](https://developer.android.com/studio)
- [Arduino IDE](https://docs.arduino.cc/software/ide/)

### Installation and Building
1. Clone the repository

2. <details><summary>Setup your hardware</summary>

    - Attach your Chassis Battery to your Wio Terminal
    ![Chassis Battery](https://files.seeedstudio.com/wiki/Wio-Terminal-Battery-Chassis/img/WT-battery-front.jpg)
    - Attach the light sensor to interface A0
    - Attach the temperature and humidity sensor to interface D2
    - Attach the sound sensor to interface A6
</details>

3. <details><summary>Setup your Arduino IDE</summary>

    - Navigate to "Boards manager"
        - Install "Arduino AVR Boards"
        - Install "Seeed SAMD Boards"
    - Navigate to "Library manager"
        - "Grove Temperature And Humidity Sensor"
        - "PubSubClient"
        - "Seeed Arduino FS"
        - "Seeed Arduino RTC"
        - "Seeed Arduino SFUD"
        - "Seeed Arduino rpcUnified"
        - "Seeed Arduino rpcWiFi"
        - "Seeed_Arduino_mbedtls"
        - "ArduinoJson"
        - "lvgl"
</details>

4. Import the project into Android Studio and build the project

5. Connect your Wio Terminal to your computer in order to boot the Wio Terminal up

## System Architecture

![Architecture Diagram.drawio.png](uploads/f1cfbd61a35ff1fc966c369338626d10/Architecture_Diagram.drawio.png){width=730 height=600}

## System Description
### Wio Terminal
The Wio Terminal measures sound intensity, lighting intensity, and the temperature and humidity of the room the user is currently in. These measurements are displayed on the Wio Terminal and sent to the Android Application through an MQTT broker. The Wio Terminal may also publish user defined uncomfortable ranges for individual sensors if the default ranges do not fit the user.

The Wio Terminal also receives user defined uncomfortable ranges for individual sensors from the Android Application through an MQTT broker.

Should the measurements exceed the ranges, the Wio Terminal will emit a noise through the built-in buzzer to alert the user about their environment not being suitable for them.