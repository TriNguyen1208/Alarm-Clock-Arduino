#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <MQ135.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "song.h"


// ========== GLOBAL VARIABLES ==========
struct AlarmSettings {
  int hour;
  int minute;
  int days[7];
  int dayCount;
  int ringtone;
} alarmSettings;

struct RTC {
  int day_index;
  int hour;
  int minute;
} rtcValue;


// ----- Topics -----
const char* ALARM_SETTINGS_TOPIC = "/alarm";
const char* ALARM_ACTIVATION_TOPIC = "/alarm/active";
const char* AIR_QUALITY_TOPIC = "/air-quality";


// ----- RTC -----
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 25200;
const long INTERVAL = 60000;


// ----- Time Handling -----
unsigned long lastTime = 0;


// ----- Alarms -----
bool isAlarmActive = true;
bool isAlarmTriggered = false;
bool NUM_ITERATIONS = 3;
// ======================================



// ========== Configurations ==========
// ----- PINOUT -----
#define DHT11_PIN 26
#define MQ135_PIN 27
#define BUZZER_PIN 32
#define BUTTON_PIN 33

// ----- WIFI -----
const char* SSID = "KP";
const char* PASSWORD = "tumotdentam";

WiFiClientSecure WIFI_CLIENT;
PubSubClient mqttClient(WIFI_CLIENT);

// ----- Initialization -----
WiFiUDP NTP_UDP;
NTPClient timeClient(NTP_UDP, NTP_SERVER, GMT_OFFSET_SEC, INTERVAL);
MQ135 mq135Sensor(MQ135_PIN);
DHT dhtSensor(DHT11_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);


// ----- Broker Server -----
const char* MQTT_SERVER = "1d9a7fc0082a411c84fa4a195fc39154.s1.eu.hivemq.cloud"; 
int PORT = 8883;
const char* MQTT_USER = "phanhuynhminhkhoi"; 
const char* MQTT_PASSWORD = "KhoiHCMUS123";


// ----- Song Player -----
SongPlayer songPlayer = SongPlayer(BUZZER_PIN, BUTTON_PIN);
// ====================================



// ========== HELPER FUNCTIONS ==========
void parseAlarmSettings(String jsonString) {
  StaticJsonDocument<512> doc;
  
  // Deserialize JSON
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("❌ JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  // Extract data from JSON
  String timeString = doc["time"].as<String>();
  int colonIndex = timeString.indexOf(':');

  alarmSettings.hour = timeString.substring(0, colonIndex).toInt();
  alarmSettings.minute = timeString.substring(colonIndex + 1).toInt();

  alarmSettings.ringtone = doc["ringtone"].as<int>();
  
  // Extract days array
  JsonArray daysArray = doc["days"];
  alarmSettings.dayCount = daysArray.size();
  
  // Copy days to struct array
  for (int i = 0; i < alarmSettings.dayCount && i < 7; i++) {
    alarmSettings.days[i] = daysArray[i];
  }
}


void triggerBuzzer() {
  Serial.println("Alarm is ringing");

  for (int i = 0; i < NUM_ITERATIONS; i++) {
    songPlayer.playSong(alarmSettings.ringtone);
  }
}
// ======================================



// ========== CONNECTION FUNCTIONS ==========
void wifiConnect() {
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
}


void mqttConnect() {
  while(!mqttClient.connected()) {
    Serial.println("Connecting MQTT");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("Connected");

      //***Subscribe all topic you need***
      mqttClient.subscribe(ALARM_SETTINGS_TOPIC);
      mqttClient.subscribe(ALARM_ACTIVATION_TOPIC);
      mqttClient.subscribe(AIR_QUALITY_TOPIC);
     
    }
    else {
      Serial.print(mqttClient.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}


void connect() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Reconnecting to WiFi");
    wifiConnect();
  }

  if(!mqttClient.connected()) {
    mqttConnect();
  }
  
  mqttClient.loop();
}


void callback(char* topic, byte* message, unsigned int length) {
  String msg;
  for(int i=0; i<length; i++) {
    msg += (char)message[i];
  }

  if (String(topic) == ALARM_ACTIVATION_TOPIC) {
    if (msg == "true") {
      isAlarmActive = true;
      Serial.println("Alarm is enabled");
    } else if (msg == "false") {
      isAlarmActive = false;
      Serial.println("Alarm is disabled");
    }
  } else if (String(topic) == ALARM_SETTINGS_TOPIC) {
    parseAlarmSettings(msg);
  }
}


void publishValue(const char* topic, float value) {
  char buffer[50];
  sprintf(buffer, "%f", value);
  mqttClient.publish(topic, buffer);
}


void publishAirQuality() {
  float ppmCo2 = mq135Sensor.getPPM();
  publishValue(AIR_QUALITY_TOPIC, ppmCo2);
}

// ==========================================


// ========== MAIN FUNCTIONS ==========
void setup() {
  Serial.begin(9600);
  // Serial.print("Connecting to WiFi");

  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  wifiConnect();

  timeClient.begin();
  timeClient.update();

  mqttClient.setServer(MQTT_SERVER, PORT);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(90);
  WIFI_CLIENT.setInsecure();
}

void loop() {
  //Connect wifi and mqtt broker
  connect();

  if (millis() - lastTime >= 1000) {
    lastTime = millis();


    // ----- Publish MQ135 -----
    publishAirQuality();
    // -------------------------


    // ----- Update RTC -----
    timeClient.update();
    rtcValue.day_index = timeClient.getDay();
    rtcValue.hour = timeClient.getHours();
    rtcValue.minute = timeClient.getMinutes();
    // ----------------------


    // ========== Debug =========
    // Serial.print("Day Index: ");
    // Serial.println(rtcValue.day_index);
    // Serial.print("Hour: ");
    // Serial.println(rtcValue.hour);
    // Serial.print("Minute: ");
    // Serial.println(rtcValue.minute);

    // Serial.print("Alarm Hour: ");
    // Serial.println(alarmSettings.hour);
    // Serial.print("Alarm Minute: ");
    // Serial.println(alarmSettings.minute);
    // ==========================


    // ----- Alarm Logic -----
    bool isDayMatch = false;
    for (int i = 0; i < alarmSettings.dayCount; i++) {
      if (alarmSettings.days[i] == rtcValue.day_index) {
        isDayMatch = true;
        break;
      }
    }

    // Trigger buzzer if day & time match
    if (isDayMatch && rtcValue.hour == alarmSettings.hour && rtcValue.minute == alarmSettings.minute && isAlarmActive == true) {
      if (isAlarmTriggered == false) {
        triggerBuzzer();
        isAlarmTriggered = true;
      }
    } else {
      isAlarmTriggered = false;
    }
    // -----------------------

  }
}
