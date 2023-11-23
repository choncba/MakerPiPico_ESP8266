/// Wifi Settings
// No necesario, ver https://github.com/JAndrassy/WiFiEspAT#persistent-wifi-connection
// #define SSID "XXXXX"
// #define PASSWORD "XXXX"

/// MQTT Settings

#define MQTT_SERVER "xxx.xxx.xxx.xxx"
#define MQTT_USER "xxxxx"
#define MQTT_PASS "xxxxx"

/// MQTT Topics

#define BASE_TOPIC "/maker_pico"
#define SET_TOPIC "/set"
#define STATE_TOPIC BASE_TOPIC "/state"
#define LWT_TOPIC BASE_TOPIC "/lwt"

#define RGB_TOPIC BASE_TOPIC "/rgb"
#define SUB_RGB_TOPIC BASE_TOPIC "/rgb/set"


#define PUB_TEMP_TOPIC BASE_TOPIC "/core_temp"