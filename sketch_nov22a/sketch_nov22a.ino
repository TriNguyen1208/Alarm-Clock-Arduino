#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

//***Set server***
const char* mqttServer = "YOUR_SERVER"; 
int port = 8883;

WiFiClientSecure wifiClient;
PubSubClient mqttClient(wifiClient);

// const char* mqttUser = "hivemq.webclient.1763802070719"; 
// const char* mqttPass = "D5a,*Wpo1F<0n?CtNX4b";


const char* mqttUser = "YOUR_USERNAME"; 
const char* mqttPass = "YOUR_PASSWORD";

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
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println(topic);
  String msg;
  for(int i=0; i<length; i++) {
    msg += (char)message[i];
  }
  Serial.println(msg);

  //***Code here to process the received package***

}

void setup() {
  Serial.begin(9600);
  Serial.print("Connecting to WiFi");

  wifiConnect();
  mqttClient.setServer(mqttServer, port);
  mqttClient.setCallback(callback);
  mqttClient.setKeepAlive( 90 );
  wifiClient.setInsecure();
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
  connect()
  //***Publish data to MQTT Server***
  int temperature = random(0,100);
  char buffer[50];
  sprintf(buffer, "%d", temperature);
  mqttClient.publish("/nothing/temperature", buffer);
  

  delay(5000);
}