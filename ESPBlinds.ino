#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include "constants.h"
#include "EasyDriver.h"

// Outputs
const int PIN_STEP   = 4;   // D2
const int PIN_DIR    = 5;   // D1
const int PIN_MS1    = -1;  // not used
const int PIN_MS2    = -1;  // not used
const int PIN_ENABLE = 14;  // D5
const int PIN_FAN    = 13;  // D7

// Inputs (not used, but reserved for future endstops)
// const int PIN_CUTOFF_CLOSE = 0;
// const int PIN_CUTOFF_OPEN  = 3;

EasyDriver stepper = EasyDriver(PIN_STEP, PIN_DIR, PIN_MS1, PIN_MS2, PIN_ENABLE);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Down = forwards == close
// Up   = reverse == open
const int DIRECTION_CLOSE = EASYDRIVER_DIRECTION_FORWARDS;
const int DIRECTION_OPEN  = EASYDRIVER_DIRECTION_REVERSE;

const bool MQTT_SEND_STEPS = false;

// EEPROM magic
int EEPROM_MAGIC_NUMBER = 10001;
int STEPS_VERTICAL = 15000;      // steps to fully open/close blinds

int DELAY_CLOSE = 500;           // µs delay when closing
int MODE_CLOSE  = EASYDRIVER_MODE_FULL_STEP;

int DELAY_OPEN = 500;            // µs delay when opening
int MODE_OPEN  = EASYDRIVER_MODE_FULL_STEP;

// Locals
int currentStep    = 0;
int currentMode    = MODE_OPEN;
int stepDirection  = EASYDRIVER_DIRECTION_FORWARDS;
bool stepperEnabled = true;
bool isOpening     = false;
bool isClosing     = false;

void setup() {
  pinMode(PIN_FAN, OUTPUT);
  stepper.reset();

  Serial.begin(115200);
  while (!Serial);

  setupWifi();
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttCallback);

  if (!mqttClient.connected()) {
    mqttReconnect();
  }

  mqttPublish(MQTT_TOPIC_ENABLED, 0);
  mqttPublish(MQTT_TOPIC_STATE, "closed");
  mqttPublish(MQTT_TOPIC_STEPS, currentStep);
  loadFromEeprom();

  // MQTT subscriptions
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_ENABLED);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_DIRECTION);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_STEPFOR);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_BLINDS);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_MODE_OPEN);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_MODE_CLOSE);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_DELAY_OPEN);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_DELAY_CLOSE);
  mqttClient.subscribe(MQTT_TOPIC_CONTROL_STEPS_VERTICAL);
}

void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
}

// ===== BLINDS CONTROL =====

void closeBlinds() {
  isClosing = true;
  mqttPublish(MQTT_TOPIC_STATE, "closing");
  setStepperDirection(DIRECTION_CLOSE);
  setStepperMode(MODE_CLOSE);
  stepFor(STEPS_VERTICAL);
  mqttPublish(MQTT_TOPIC_STATE, "closed");
  isClosing = false;
}

void openBlinds() {
  isOpening = true;
  mqttPublish(MQTT_TOPIC_STATE, "opening");
  setStepperDirection(DIRECTION_OPEN);
  setStepperMode(MODE_OPEN);
  stepFor(STEPS_VERTICAL); 
  mqttPublish(MQTT_TOPIC_STATE, "opened");
  isOpening = false;
}

// ===== STEPPER HELPERS =====

void stepFor(int steps) {
  setStepperEnabled(true);
  for (int i = 0; i < steps; i++) {
    for (int x = 0; x < currentMode; x++) {
      stepper.step();
    }
    currentStep += stepDirection;

    if (!stepperEnabled) break;

    if (MQTT_SEND_STEPS && (currentStep % 250 == 0)) {
      mqttPublish(MQTT_TOPIC_STEPS, currentStep);
    }
    mqttClient.loop();
  }
  setStepperEnabled(false);
}

void setStepperMode(int mode) {
  stepper.setMode(mode);
  currentMode = mode;
  mqttPublish(MQTT_TOPIC_MODE, mode);
}

void setStepperEnabled(bool enabled) {
  stepper.enable(enabled);
  if (enabled) {
    digitalWrite(PIN_FAN, HIGH);
    mqttPublish(MQTT_TOPIC_ENABLED, 1); 
  } else {
    digitalWrite(PIN_FAN, LOW);
    mqttPublish(MQTT_TOPIC_ENABLED, 0); 
  }
  stepperEnabled = enabled;
}

void setStepperDirection(int direction) {
  stepper.setDirection(direction);
  stepDirection = direction;
  if (direction == EASYDRIVER_DIRECTION_FORWARDS) {
    stepper.setDelay(DELAY_CLOSE);
    mqttPublish(MQTT_TOPIC_DIRECTION, "forwards");
  } else {
    stepper.setDelay(DELAY_OPEN);
    mqttPublish(MQTT_TOPIC_DIRECTION, "reverse");
  }
}

// ===== WIFI =====

void setupWifi() {
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// ===== MQTT =====

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String topicStr = topic;
  String value = "";
  for (int i = 0; i < length; i++) {
      value += (char)payload[i];
  }

  Serial.println("mqttCallback:");
  Serial.println(topicStr);
  Serial.println(value);

  if (topicStr == MQTT_TOPIC_CONTROL_ENABLED) {
    if (value.equals("1")) setStepperEnabled(true);
    else if (value.equals("0")) setStepperEnabled(false);
  }

  if (topicStr == MQTT_TOPIC_CONTROL_DIRECTION) {
    if (value.equals("1")) setStepperDirection(DIRECTION_CLOSE);
    else if (value.equals("0")) setStepperDirection(DIRECTION_OPEN);
  }

  if (topicStr == MQTT_TOPIC_CONTROL_STEPFOR) {
    int steps = value.toInt();
    stepFor(steps);
  }

  if (topicStr == MQTT_TOPIC_CONTROL_BLINDS) {
    if (value.equals("opened") && !isOpening) openBlinds();
    else if (value.equals("closed") && !isClosing) closeBlinds();
  }

  if (topicStr == MQTT_TOPIC_CONTROL_MODE_OPEN) {
    MODE_OPEN = value.toInt(); saveToEeprom();
  }
  if (topicStr == MQTT_TOPIC_CONTROL_MODE_CLOSE) {
    MODE_CLOSE = value.toInt(); saveToEeprom();
  }
  if (topicStr == MQTT_TOPIC_CONTROL_DELAY_OPEN) {
    DELAY_OPEN = value.toInt(); saveToEeprom();
  }
  if (topicStr == MQTT_TOPIC_CONTROL_DELAY_CLOSE) {
    DELAY_CLOSE = value.toInt(); saveToEeprom();
  }
  if (topicStr == MQTT_TOPIC_CONTROL_STEPS_VERTICAL) {
    STEPS_VERTICAL = value.toInt(); saveToEeprom();
  }
}

void mqttReconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD, MQTT_TOPIC_STATUS, 1, true, "disconnected", false)) {
      Serial.println("connected");
      mqttClient.publish(MQTT_TOPIC_STATUS, "connected", true);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttPublish(char *topic, int payload) {
  mqttClient.publish(topic, String(payload).c_str(), true);
}

void mqttPublish(char *topic, String payload) {
  mqttClient.publish(topic, payload.c_str(), true);
}

// ===== EEPROM =====

void loadFromEeprom() {
  EEPROM.begin(24);
  int magicNumber, modeOpen, modeClose, delayOpen, delayClose, stepsVertical;

  EEPROM.get(0, modeOpen);
  EEPROM.get(4, modeClose);
  EEPROM.get(8, delayOpen);
  EEPROM.get(12, delayClose);
  EEPROM.get(16, stepsVertical);
  EEPROM.get(20, magicNumber);
  EEPROM.end();

  if (magicNumber == EEPROM_MAGIC_NUMBER) {
    MODE_OPEN = modeOpen;
    MODE_CLOSE = modeClose;
    DELAY_OPEN = delayOpen;
    DELAY_CLOSE = delayClose;
    STEPS_VERTICAL = stepsVertical;
    Serial.println("EEPROM config loaded");
  } else {
    Serial.println("EEPROM invalid, using defaults");
  }

  mqttPublish(MQTT_TOPIC_MODE_OPEN, MODE_OPEN);
  mqttPublish(MQTT_TOPIC_MODE_CLOSE, MODE_CLOSE);
  mqttPublish(MQTT_TOPIC_DELAY_OPEN, DELAY_OPEN);
  mqttPublish(MQTT_TOPIC_DELAY_CLOSE, DELAY_CLOSE);
  mqttPublish(MQTT_TOPIC_STEPS_VERTICAL, STEPS_VERTICAL);
}

void saveToEeprom() {
  EEPROM.begin(24);
  EEPROM.put(0, MODE_OPEN);
  EEPROM.put(4, MODE_CLOSE);
  EEPROM.put(8, DELAY_OPEN);
  EEPROM.put(12, DELAY_CLOSE);
  EEPROM.put(16, STEPS_VERTICAL);
  EEPROM.put(20, EEPROM_MAGIC_NUMBER);
  EEPROM.commit();
  EEPROM.end();

  mqttPublish(MQTT_TOPIC_MODE_OPEN, MODE_OPEN);
  mqttPublish(MQTT_TOPIC_MODE_CLOSE, MODE_CLOSE);
  mqttPublish(MQTT_TOPIC_DELAY_OPEN, DELAY_OPEN);
  mqttPublish(MQTT_TOPIC_DELAY_CLOSE, DELAY_CLOSE);
  mqttPublish(MQTT_TOPIC_STEPS_VERTICAL, STEPS_VERTICAL);
}
