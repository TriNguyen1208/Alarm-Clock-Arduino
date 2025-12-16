#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <MQ135.h>
#include <DHT.h>
#include <Wire.h>
#include <time.h>
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
  int second;
} rtcValue;

struct Measurements {
  float ppm;
  float temperature;
  float humidity;
} measurementObj;

// ----- Topics -----
const char* ALARM_SETTINGS_TOPIC = "/alarm";
const char* ALARM_ACTIVATION_TOPIC = "/alarm/active";
const char* AIR_QUALITY_TOPIC = "/air-quality";
const char* TEMPERATURE_TOPIC = "/temperature";
const char* HUMIDITY_TOPIC = "/humidity";


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


// ----- LCD -----
bool lcdMode = true;

byte TEMP_ICON[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110
};

byte HUMID_ICON[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b10001,
  0b01110
};


// ----- BUTTON -----
unsigned long pressedTime = 0.0;
int lastState = LOW;
// ======================================



// ========== Configurations ==========
// ----- PINOUT -----
#define LED_PIN 25
#define DHT11_PIN 26
#define BUTTON_PIN 27
#define BUZZER_PIN 32
#define MQ135_PIN 33
#define POTEN_PIN 35

// ----- WIFI -----
const char* SSID = "your_SSID";
const char* PASSWORD = "your_password";

WiFiClientSecure WIFI_CLIENT;
PubSubClient mqttClient(WIFI_CLIENT);

// ----- Initialization -----
WiFiUDP NTP_UDP;
NTPClient timeClient(NTP_UDP, NTP_SERVER, GMT_OFFSET_SEC, INTERVAL);
MQ135 mq135Sensor(MQ135_PIN);
DHT dhtSensor(DHT11_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);


// ----- Broker Server -----
const char* MQTT_SERVER = "your_broker"; 
int PORT = 8883;
const char* MQTT_USER = "your_mqtt_username"; 
const char* MQTT_PASSWORD = "your_mqtt_password";


// ----- Song Player -----
SongPlayer songPlayer = SongPlayer(BUZZER_PIN, BUTTON_PIN);
// ====================================



// ========== HELPER FUNCTIONS ==========
void parseAlarmSettings(String jsonString) {
  StaticJsonDocument<512> doc;
  
  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("❌ JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }
  
  String timeString = doc["time"].as<String>();
  int colonIndex = timeString.indexOf(':');

  alarmSettings.hour = timeString.substring(0, colonIndex).toInt();
  alarmSettings.minute = timeString.substring(colonIndex + 1).toInt();

  alarmSettings.ringtone = doc["ringtone"].as<int>();
  
  JsonArray daysArray = doc["days"];
  alarmSettings.dayCount = daysArray.size();
  
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


void publishValue(const char* topic, float value) {
  char buffer[50];
  sprintf(buffer, "%.1f", value);
  mqttClient.publish(topic, buffer);
}


void publishAirQuality() {
  float ppmCo2 = mq135Sensor.getPPM();
  measurementObj.ppm = ppmCo2;
  publishValue(AIR_QUALITY_TOPIC, ppmCo2);
}


void publishTemperature() {
  float temp = dhtSensor.readTemperature();
  measurementObj.temperature = temp;
  publishValue(TEMPERATURE_TOPIC, temp);
}


void publishHumidity() {
  float humid = dhtSensor.readHumidity();
  measurementObj.humidity = humid;
  publishValue(HUMIDITY_TOPIC, humid);
}


void displayLcd(bool lcdMode) {
  if (lcdMode) {
    lcd.setCursor(0, 0);
    lcd.print("Time: ");
    if(rtcValue.hour < 10) lcd.print("0"); lcd.print(rtcValue.hour);
    lcd.print(":");
    if(rtcValue.minute < 10) lcd.print("0"); lcd.print(rtcValue.minute);
    lcd.print(":");
    if(rtcValue.second < 10) lcd.print("0"); lcd.print(rtcValue.second);
  
    time_t epochTime = timeClient.getEpochTime();  
    struct tm *ptm = localtime(&epochTime);     

    int day = ptm->tm_mday;
    int month = ptm->tm_mon + 1;
    int year = ptm->tm_year + 1900;

    // Hiển thị ngày
    lcd.setCursor(0, 1);
    lcd.print("Date: ");
    if(day < 10) lcd.print("0");
    lcd.print(day);
    lcd.print("/");
    if(month < 10) lcd.print("0");
    lcd.print(month);
    lcd.print("/");
    lcd.print(year);

  } else {
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.write((byte) 0);
    lcd.print(" ");
    lcd.print(measurementObj.temperature, 1);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(0,1);
    lcd.print("Humidity: ");
    lcd.write((byte) 1);
    lcd.print(" ");
    lcd.print(measurementObj.humidity, 0);
    lcd.print("%");
  }
}
// ======================================



// ========== CONNECTION FUNCTIONS ==========
void wifiConnect() {
  WiFi.mode(WIFI_STA);
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
// ==========================================


// ========== MAIN FUNCTIONS ==========
void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");

  pinMode(BUTTON_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(POTEN_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  wifiConnect();

  analogReadResolution(10);

  timeClient.begin();
  timeClient.forceUpdate();

  mqttClient.setServer(MQTT_SERVER, PORT);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive(90);
  WIFI_CLIENT.setInsecure();

  dhtSensor.begin();
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, TEMP_ICON);
  lcd.createChar(1, HUMID_ICON);
}


void loop() {
  // ----- Connect Wifi and MQTT -----
  connect();
  // ---------------------------------

  if (millis() - lastTime >= 1000) {

    lastTime = millis();

    // ----- Publish Sensors -----
    publishAirQuality();
    publishHumidity();
    publishTemperature();
    // -------------------------


    // ----- Update RTC -----
    timeClient.update();
    rtcValue.day_index = timeClient.getDay();
    rtcValue.hour = timeClient.getHours();
    rtcValue.minute = timeClient.getMinutes();
    rtcValue.second = timeClient.getSeconds();
    // ----------------------


    // ----- LCD Display -----
    displayLcd(lcdMode);
    // -----------------------


    // ----- Alarm Logic -----
    bool isDayMatch = false;
    for (int i = 0; i < alarmSettings.dayCount; i++) {
      if (alarmSettings.days[i] == rtcValue.day_index) {
        isDayMatch = true;
        break;
      }
    }

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


  // ----- Potentiometer Logic -----
  int potenValue = analogRead(POTEN_PIN);
  int ledValue = map(potenValue, 0, 1023, 0, 255);
  analogWrite(LED_PIN, ledValue);
  // -------------------------------


  // ----- Button Logic -----
  int stateButton = digitalRead(BUTTON_PIN);
  if (lastState == LOW && stateButton == HIGH) {
    pressedTime = millis();
  }

  if (lastState == HIGH && stateButton == LOW) {
    if (millis() - pressedTime >= 1500) {
      lcd.clear();
      lcdMode = !lcdMode;
    }
  } 
  // ------------------------
  

  lastState = stateButton;
}
