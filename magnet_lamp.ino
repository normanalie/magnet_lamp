#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

/* MQTT Config */
#include "mqtt_conf.h"
/*
#define MQTT_SERVER   "<server address>"
#define MQTT_PORT     <port>
#define MQTT_USER     "<user>"
#define MQTT_PASSWORD "<password>"
*/

#define DATA_PIN 3
#define NUM_LEDS 2

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB+NEO_KHZ800);

WiFiClientSecure espClient;
WiFiManager wm;

PubSubClient mqtt_client(espClient);
String mqtt_client_id = "pcboflight_";


void wifi_setup();
void mqtt_setup();
void mqtt_reconnect();
void mqtt_callback(char *topic, byte *payload, unsigned int len);
void setBrightness(uint8_t value);
void setColor(uint8_t r, uint8_t g, uint8_t b);

void setup(){
  Serial.begin(9600);
  delay(1000);
  Serial.println("======== PCB Of Light ========");

  espClient.setInsecure();

  wifi_setup();
  mqtt_setup();

 strip.begin();
 strip.show();
 strip.setBrightness(50);
}


void loop(){
  static unsigned long t = millis();
  if(millis()-t > 100){
    strip.show();
    t = millis();
  }
    wm.process(); 
    if(!mqtt_client.connected()){
      mqtt_reconnect();
    } else {
      mqtt_client.loop();
    }
}



void wifi_setup(){
  WiFi.mode(WIFI_STA);
  wm.setConfigPortalBlocking(false);
  wm.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));  // Capttive portal IP
  
  // UI
  wm.setTitle("PCB Of Light");
  wm.setClass("invert");  // Dark mode
  
  // Start
  wm.autoConnect("PCBOfLight_Setup");
  return;
}


void mqtt_setup(){
  mqtt_client_id += String(random(0xffff), HEX);  // TODO: Replace by unique id based on Serial Number
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(mqtt_callback);
  return;
}


void mqtt_reconnect(){
  if(!mqtt_client.connected()){
    Serial.println("[MQTT] - Reconnecting...");
    if(mqtt_client.connect(mqtt_client_id.c_str(), MQTT_USER, MQTT_PASSWORD)){  // Attempt to connect
      Serial.println("MQTT Connected");
      mqtt_client.publish("pcboflight/test", "Hello from ESP");
      mqtt_client.subscribe("pcboflight/test/set");
    } else {
      Serial.println("MQTT Failed to connect");
      Serial.println(mqtt_client.state());
    }    
  }
}

void mqtt_callback(char *topic, byte *payload, unsigned int len){
  StaticJsonDocument<200> data;
  Serial.print("[MQTT] - Message on [");
  Serial.print(topic);
  Serial.println("]");
  //Serial.println((char*)payload);
  DeserializationError err = deserializeJson(data, (char *)payload);
  if(err){
    Serial.println("Deserialization error");
    Serial.println(err.f_str());
  } else {
    uint8_t r=(uint8_t)data["r"];
    uint8_t g=(uint8_t)data["g"];
    uint8_t b=(uint8_t)data["b"];
    uint8_t br=data["brightness"];
    setBrightness(br);
    setColor(r,g,b);    
  }
  return;
}


void setBrightness(uint8_t value){
  strip.setBrightness(value);
  strip.show();
  char json[30] = "";
  sprintf(json, "{\"brightness\":%d}\0", value);
  mqtt_client.publish("pcboflight/test", json);
}

void setColor(uint8_t r, uint8_t g, uint8_t b){
  Serial.println(r);
  Serial.println(g);
  Serial.println(b);
  for(int c=0; c<NUM_LEDS; c++){
    strip.setPixelColor(c, r, g, b);
  }
  strip.show();
  char json[200] = "";
  sprintf(json, "{\"r\":%d, \"g\":%d, \"b\":%d}\0", r, g, b);
  mqtt_client.publish("pcboflight/test", json);
}