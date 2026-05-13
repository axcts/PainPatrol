#define LV_HOR_RES_MAX (320)
#define LV_VER_RES_MAX (240)

#define DHT_TYPE DHT11  // DHT 11
#define DHT_PIN 2 // is the TOPRIGHT one
#define LIGHT_PIN A0 // TOPLEFT
#define SOUND_PIN A6 // BOTTOMLEFT
#define TIMEZONE_OFFSET (7200)

#define LIGHT_MIN 10.0f
#define LIGHT_MAX 60.0f
#define SOUND_MIN 0.0f
#define SOUND_MAX 50.0f
#define TEMP_MIN 18.0f
#define TEMP_MAX 25.0f
#define HUMIDITY_MIN 30.0f
#define HUMIDITY_MAX 60.0f

#define BUZZER_COOLDOWN 90000 //ms, 2 mins
#define BUZZ_TIME 400 //ms, .2s
#define BUZZ_AMOUNT 3

#define SENSOR_AMOUNT 4
