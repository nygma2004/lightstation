// Weather station for light VEML6070 and SI1145 sensors for light, infrared, and UV measurement
// 4 channel repay output on D5, D6, D7, D0

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>           // MQTT support
#include <ESP8266WiFi.h>
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VEML6070.h"
#include "Adafruit_SI1145.h"
#include <Adafruit_Sensor.h>
extern "C" {
#include "user_interface.h"
}

#define LENG 31   //0x42 + 31 bytes equal to 32 bytes
#define SEALEVELPRESSURE_HPA (1013.25)
#define RELAY1 14         // D5 pin
#define RELAY2 13         // D6 pin
#define RELAY3 12         // D7 pin
#define RELAY4 16         // D0 pin
#define LEDPIN 2          // D4 Wemos blue status led

// Update the below parameters for your project
const char* ssid = "xxx";
const char* password = "xxx";
const char* mqtt_server = "192.168.1.xx"; 
const char* mqtt_user = "xxx";
const char* mqtt_password = "xxx";
const char* clientID = "lightstation";
const char* topicStatus = "lightstation/status";
const char* topicUVI = "lightstation/uvi";
const char* topicUVIndex = "lightstation/uvindex";
const char* topicLight = "lightstation/light";
const char* topicIR = "lightstation/infrared";
const char* topicDebug = "lightstation/debug";
const char* inTopic = "lightstation/#";
const char* topicSetR1 = "lightstation/relay1";
const char* topicSetR2 = "lightstation/relay2";
const char* topicSetR3 = "lightstation/relay3";
const char* topicSetR4 = "lightstation/relay4";
const char* topicStatusR1 = "lightstation/relay1status";
const char* topicStatusR2 = "lightstation/relay2status";
const char* topicStatusR3 = "lightstation/relay3status";
const char* topicStatusR4 = "lightstation/relay4status";

unsigned char buf[LENG];
unsigned long lastTick, uptime, seconds, pmsNext;
char msg[50];
String message = "";
String webPage = "";
String webStat = "";
String webSensors = "";
String webFooter = "";
String mqttStat = "";
bool stateRelay1, stateRelay2, stateRelay3, stateRelay4;

Adafruit_VEML6070 uv = Adafruit_VEML6070();
Adafruit_SI1145 uva = Adafruit_SI1145();

os_timer_t myTimer;
MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiClient espClient;
PubSubClient mqtt(mqtt_server, 1883, 0, espClient);

bool setRelay(int relay, int state) {
  switch (relay) {
    case 1:
      stateRelay1 = state;
      digitalWrite(RELAY1, !stateRelay1);
      mqtt.publish(topicStatusR1, (state ? "1" : "0"));
      refreshStats();  
      return true;
      break;
    case 2:
      stateRelay2 = state;
      digitalWrite(RELAY2, !stateRelay2);
      mqtt.publish(topicStatusR2, (state ? "1" : "0"));
      refreshStats();  
      return true;
      break;
    case 3:
      stateRelay3 = state;
      digitalWrite(RELAY3, !stateRelay3);
      mqtt.publish(topicStatusR3, (state ? "1" : "0"));
      refreshStats();  
      return true;
      break;
    case 4:
      stateRelay4 = state;
      digitalWrite(RELAY4, !stateRelay4); 
      mqtt.publish(topicStatusR4, (state ? "1" : "0"));
      refreshStats();  
      return true;     
      break;
    default:
      return false;
  }
}

// This routing just puts the status string together which will be sent over MQTT
void refreshStats() {
  // Initialize the strings for MQTT 
  mqttStat = "{\"rssi\":";
  mqttStat += WiFi.RSSI();
  webStat = "<p style=\"font-size: 90%; color: #FF8000;\">RSSI: ";
  webStat += WiFi.RSSI();
  webStat += "<br/>";
  
  mqttStat += ",\"uptime\":";
  mqttStat += uptime;
  webStat += "Uptime [min]: ";
  webStat += uptime;
  webStat += "<br/>";

  mqttStat += ",\"relays\":\"";
  mqttStat += stateRelay1;
  mqttStat += stateRelay2;
  mqttStat += stateRelay3;
  mqttStat += stateRelay4;
  mqttStat += "\"";
  mqttStat += "}";
  webStat += "Relay1 state: ";
  webStat += stateRelay1;
  webStat += "<br/>";

  webStat += "Relay2 state: ";
  webStat += stateRelay2;
  webStat += "<br/>";

  webStat += "Relay3 state: ";
  webStat += stateRelay3;
  webStat += "<br/>";

  webStat += "Relay4 state: ";
  webStat += stateRelay4;
  webStat += "<br/>";


  webStat += "</p>";


}

// This is the 1 second timer callback function
uint8_t sec=0;
void timerCallback(void *pArg) {
  sec++;
  seconds++;
  if (seconds==10) {
    // Send MQTT update
    readSensors();
    refreshStats();
    if (mqtt_server!="") {
      mqtt.publish(topicStatus, mqttStat.c_str());
      Serial.print(F("Status: "));
      Serial.println(mqttStat);
    }
    
    seconds = 0;
  }
}

// MQTT reconnect logic
void reconnect() {
  //String mytopic;
  // Loop until we're reconnected
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (mqtt.connect(clientID, mqtt_user, mqtt_password)) {
      Serial.println(F("connected"));
      // ... and resubscribe
      mqtt.subscribe(inTopic);
    } else {
      Serial.print(F("failed, rc="));
      Serial.print(mqtt.state());
      Serial.println(F(" try again in 5 seconds"));
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  // Convert the incoming byte array to a string
  String mytopic;
  digitalWrite(LEDPIN, LOW);
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;

  Serial.print("Message arrived on topic: [");
  Serial.print(topic);
  Serial.print("], ");
  Serial.println(message);

  if (strcmp(topic,topicSetR1)==0) {
    setRelay(1,message.toInt());
  }
  if (strcmp(topic,topicSetR2)==0) {
    setRelay(2,message.toInt());
  }
  if (strcmp(topic,topicSetR3)==0) {
    setRelay(3,message.toInt());
  }
  if (strcmp(topic,topicSetR4)==0) {
    setRelay(4,message.toInt());
  }
  digitalWrite(LEDPIN, HIGH);

}

void handleRelayCommand() { 
  String message = "";
  int relay = 2;
  bool state = false;
  digitalWrite(LEDPIN, LOW);

  if (server.hasArg("relay")) {
    relay = server.arg("relay").toInt();
  }
  if (server.hasArg("state")) {
    state = (bool)server.arg("state").toInt();
  }
  if (!setRelay(relay,state)) {
    message = "Incorrect parameters";
  }

  if (message=="") {
    message = "Relay ";
    message += relay;
    message += " set to state ";
    message += state;
  }
  server.send(200, "text/plain", message);          //Returns the HTTP response
  Serial.print("HTTP relay request: ");
  Serial.println(message);
  digitalWrite(LEDPIN, HIGH);
  
}

void setup()
{
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println("Weather station: Si1145, VEML6959");
  uptime = 0;
  sec = 0;
  seconds = 0;

  pinMode(LEDPIN, OUTPUT);  // Status LED
  digitalWrite(LEDPIN, HIGH);
  stateRelay1 = LOW;
  pinMode(RELAY1, OUTPUT);  // Relay for the pump
  digitalWrite(RELAY1, !stateRelay1);
  stateRelay2 = LOW;
  pinMode(RELAY2, OUTPUT);  // Relay2
  digitalWrite(RELAY2, !stateRelay2);
  stateRelay3 = LOW;
  pinMode(RELAY3, OUTPUT);  // Relay3
  digitalWrite(RELAY3, !stateRelay3);
  stateRelay4 = LOW;
  pinMode(RELAY4, OUTPUT);  // Relay4
  digitalWrite(RELAY4, !stateRelay4);

  webPage = "<html><body><h1>ESP8266 Light Weather Station</h1><p>This sketch gets sensor readings from Si1145, VEML6070 and publishes them to MQTT.</p>";

  webPage += "<p><b>HTTP Relay Commands:</b><br/><i>/relay?relay=x&amp;state=y</i>: set the relay status, x is 1-4, y is 1 or 0</p>";
  webPage += "<p><b>MQTT Relay Commands:</b><br/><i>root_topic/relayx</i>: x is 1-4, payload is 1 or 0</p>";

  webSensors = "<p style=\"font-size: 90%; color: #007FFF;\">No sensor readings available yet.</p>";
  webStat = "<p style=\"font-size: 90%; color: #FF8000;\">No stats available yet.</p>";
  webFooter = "<p style=\"font-size: 80%; color: #08088A;\">ESP8266 Weather Station v2.0 | <a href=\"mailto:csongor.varga@gmail.com\">email me</a></p></body></html>";


  Serial.print(F("Connecting to Wifi"));
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    seconds++;
    if (seconds>180) {
      // reboot the ESP if cannot connect to wifi
      ESP.restart();
    }
  }
  seconds = 0;
  Serial.println("");
  Serial.print(F("Connected to "));
  Serial.println(ssid);
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());
  Serial.print(F("Signal [RSSI]: "));
  Serial.println(WiFi.RSSI());

  os_timer_setfn(&myTimer, timerCallback, NULL);
  os_timer_arm(&myTimer, 1000, true);

  // Set up the MDNS and the HTTP server
  if (mdns.begin("Weather", WiFi.localIP())) {
    Serial.println(F("MDNS responder started"));
  }  
  server.on("/", [](){                        // landing page
    server.send(200, "text/html", webPage+webSensors+webStat+webFooter);
  });
  server.on("/relay", handleRelayCommand);    // Handling parameter set
  server.begin();
  Serial.println(F("HTTP server started"));  

  // Set up the MQTT server connection
  if (mqtt_server!="") {
    mqtt.setServer(mqtt_server, 1883);
    mqtt.setCallback(callback);
  }

  uv.begin(VEML6070_1_T);  // pass in the integration time constant
  Serial.println("VEML6070 initialized");
  if (! uva.begin()) {
    Serial.println("Didn't find Si1145");
  } else {
    Serial.println("Si1145 initialized");
  }

}
 
void loop() {

  // Handle HTTP server requests
  server.handleClient();

  // Handle MQTT connection/reconnection
  if (mqtt_server!="") {
    if (!mqtt.connected()) {
      reconnect();
    }
    mqtt.loop();
  }

  // Uptime calculation
  if (millis() - lastTick >= 60000) {            
    lastTick = millis();            
    uptime++;            
  }    

 
}

void readSensors() {

  // VEML6070 UV sensor
  webSensors= "<p style=\"font-size: 90%; color: #007FFF;\"><b>VEML6070 UV sensor</b><br/>";
  long uvi = uv.readUV();
  Serial.print("UVI            : "); 
  Serial.println(uvi);
  mqtt.publish(topicUVI, String(uvi).c_str());
  webSensors+= "UVI: ";
  webSensors+= uvi;
  webSensors+= "<br/>";  
  Serial.print("UV index       : "); 
  Serial.println(uvi / 187);
  mqtt.publish(topicUVIndex, String(uvi / 187).c_str());
  webSensors+= "UV Index: ";
  webSensors+= uvi / 187;
  webSensors+= "</p>";  
  
  // Si1145 Light sensor
  webSensors+= "<p style=\"font-size: 90%; color: #007FFF;\"><b>Si1145 light sensor</b><br/>";
  long light = uva.readVisible();
  Serial.print("Light intensity: "); Serial.println(light);
  mqtt.publish(topicLight, String(light).c_str());
  webSensors+= "Light intensity: ";
  webSensors+= light;
  webSensors+= "<br/>";  
  long ir = uva.readIR();
  Serial.print("Infra red      : "); Serial.println(ir);
  mqtt.publish(topicIR, String(ir).c_str());
  webSensors+= "IR intensity: ";
  webSensors+= ir;
  webSensors+= "</p>";  
  
  webSensors+= "<p style=\"font-size: 90%; color: #007FFF;\">Sensor data does not auto update on this page</p>"; 
}
  

