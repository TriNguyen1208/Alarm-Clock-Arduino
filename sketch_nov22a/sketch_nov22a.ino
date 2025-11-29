#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include "DHT.h" // Sử dụng cho cảm biến nhiệt độ + độ ẩm
#include <Wire.h> //
#include <LiquidCrystal_I2C.h> // Sử dụng cho cảm biến đèn LED
#include <NTPClient.h> // Sử dụng để hiển thị thời gian cho đèn LED
#include <WiFiUdp.h> 
#include <time.h>
#define button 5
#define DHTPIN 14
#define DHTTYPE DHT22
#define potent 32
// #define LED 2
// #define ledChannel 0
// #define freq 5000
// #define resolution 8
const int LED = 2;        // chân LED
const int freq = 5000;    // tần số PWM
const int resolution = 8; // 8 bit
int potent_val;
int LED_val;
// Những biến tham gia vào tính thời gian nút bấm 
int time_now;
int time_click = 0;
int lastState = LOW;
int stateButton;
///
DHT dht(DHTPIN, DHTTYPE);
const char* ssid = "Wokwi-GUEST";
const char* password = "";
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7*3600, 60000); 
//***Set server***
const char* mqttServer = "1d9a7fc0082a411c84fa4a195fc39154.s1.eu.hivemq.cloud"; 
int port = 8883;

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

const char* mqttUser = "luongbahoangtu"; 
const char* mqttPass = "TuHCMUS123";

bool modLed = true;
void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
}

void mqttConnect() {
  while(!mqttClient.connected()) {
    Serial.println("Attemping MQTT connection...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if(mqttClient.connect(clientId.c_str(), mqttUser, mqttPass)) {
      Serial.println("connected");

      //***Subscribe all topic you need***
      // Những topic là những thông tin nào mà mạch cần nhận từ web
      mqttClient.subscribe("/nothing/temperature");
     
    }
    else {
      Serial.print(mqttClient.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

//MQTT Receiver
// Những thông tin mà mạch nhận được từ website
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println(topic);
  String msg;
  for(int i=0; i<length; i++) {
    msg += (char)message[i];
  }
  Serial.println(msg);

  //***Code here to process the received package***

}


byte tempIcon[8] = {
  0b00100,
  0b01010,
  0b01010,
  0b01110,
  0b01110,
  0b11111,
  0b11111,
  0b01110
};

byte humIcon[8] = {
  0b00100,
  0b00100,
  0b01010,
  0b01010,
  0b10001,
  0b10001,
  0b10001,
  0b01110
};
void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");
  dht.begin();
  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );
  wifiClient.setInsecure();
  lcd.init();
  lcd.backlight();
  timeClient.begin();
  pinMode(button, INPUT);
  pinMode(potent, INPUT);
  ledcAttach(LED, freq, resolution);

  lcd.createChar(0, tempIcon);
  lcd.createChar(1, humIcon);
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

void loop() {
  //Connect wifi and mqtt broker
  connect();
  
  // Hàm thay đổi chế độ màn hình LCD 16x2
  time_now = millis();
  stateButton = digitalRead(button);
  if (lastState == LOW && stateButton == HIGH) 
  {
    time_click = millis();
  }
  if (lastState == HIGH && stateButton == LOW)
  {
    if (time_now - time_click >= 1000)
    {
       lcd.clear();
       modLed = !modLed;
    }
  } 
  lastState = stateButton;
  //

  // //***Publish data to MQTT Server***
  char buffer[50];
  float humid = dht.readHumidity();
  sprintf(buffer, "%.1f", humid);
  mqttClient.publish("ESP/humid", buffer);
  float temp = dht.readTemperature();
  sprintf(buffer, "%.1f", temp);
  mqttClient.publish("ESP/temp",buffer); 
  //

 

  // // Hiển thị trên LCD
  if (modLed) 
  {
    timeClient.update();
    int h = timeClient.getHours();
    int m = timeClient.getMinutes();
    int s = timeClient.getSeconds();

    // Hiển thị giờ
    lcd.setCursor(0,0);
    lcd.print("Time: ");
    if(h<10) lcd.print("0"); lcd.print(h);
    lcd.print(":");
    if(m<10) lcd.print("0"); lcd.print(m);
    lcd.print(":");
    if(s<10) lcd.print("0"); lcd.print(s);

  
    time_t epochTime = timeClient.getEpochTime();  
    struct tm *ptm = localtime(&epochTime);     

    int day = ptm->tm_mday;
    int month = ptm->tm_mon + 1;
    int year = ptm->tm_year + 1900;

    // Hiển thị ngày
    lcd.setCursor(0,1);
    if(day<10) lcd.print("0");
    lcd.print(day);
    lcd.print("/");
    if(month<10) lcd.print("0");
    lcd.print(month);
    lcd.print("/");
    lcd.print(year);

  
  }
  else 
  {
    lcd.setCursor(0,0);
    lcd.print("Temp: ");
    lcd.write((byte)0);
    lcd.print(" ");
    lcd.print(temp, 1);
    lcd.print((char)223);
    lcd.print("C");

    lcd.setCursor(0,1);
    lcd.print("Humidity: ");
    lcd.write((byte)1);
    lcd.print(" ");
    lcd.print(humid, 0);
    lcd.print("%");
  }
  //
  // Dùng biến trở điều chỉnh độ sáng của đèn 
  potent_val = analogRead(potent);
  LED_val = (int)((potent_val / 4095.0) * 255);
  ledcWrite(LED, LED_val);
  //
}
