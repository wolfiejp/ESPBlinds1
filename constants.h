// ===== WiFi =====
const char *WIFI_SSID     = "ZyXEL_ECAD";
const char *WIFI_PASSWORD = "95greeNlizarD";

// ===== MQTT =====
const char *MQTT_SERVER   = "192.168.1.50";   // change to your laptop IP
const char *MQTT_USER     = "";
const char *MQTT_PASSWORD = "";

#define MQTT_CLIENT_ID "esp8266_blinds_01"

// Status
#define MQTT_TOPIC_STATUS "home/esp8266_blinds_01/status"

// Control
#define MQTT_TOPIC_CONTROL_ENABLED        "home/esp8266_blinds_01/control/enabled"
#define MQTT_TOPIC_CONTROL_DIRECTION      "home/esp8266_blinds_01/control/direction"
#define MQTT_TOPIC_CONTROL_STEPFOR        "home/esp8266_blinds_01/control/stepfor"
#define MQTT_TOPIC_CONTROL_BLINDS         "home/esp8266_blinds_01/control/blinds"
#define MQTT_TOPIC_CONTROL_MODE_OPEN      "home/esp8266_blinds_01/control/mode_open"
#define MQTT_TOPIC_CONTROL_MODE_CLOSE     "home/esp8266_blinds_01/control/mode_close"
#define MQTT_TOPIC_CONTROL_DELAY_OPEN     "home/esp8266_blinds_01/control/delay_open"
#define MQTT_TOPIC_CONTROL_DELAY_CLOSE    "home/esp8266_blinds_01/control/delay_close"
#define MQTT_TOPIC_CONTROL_STEPS_VERTICAL "home/esp8266_blinds_01/control/steps_vertical"

// State
#define MQTT_TOPIC_ENABLED        "home/esp8266_blinds_01/enabled"
#define MQTT_TOPIC_DIRECTION      "home/esp8266_blinds_01/direction"
#define MQTT_TOPIC_MODE           "home/esp8266_blinds_01/mode"
#define MQTT_TOPIC_STATE          "home/esp8266_blinds_01/state"
#define MQTT_TOPIC_STEPS          "home/esp8266_blinds_01/steps"
#define MQTT_TOPIC_MODE_OPEN      "home/esp8266_blinds_01/mode_open"
#define MQTT_TOPIC_MODE_CLOSE     "home/esp8266_blinds_01/mode_close"
#define MQTT_TOPIC_DELAY_OPEN     "home/esp8266_blinds_01/delay_open"
#define MQTT_TOPIC_DELAY_CLOSE    "home/esp8266_blinds_01/delay_close"
#define MQTT_TOPIC_STEPS_VERTICAL "home/esp8266_blinds_01/steps_vertical"
