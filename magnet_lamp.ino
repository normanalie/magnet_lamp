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
#define NUM_LEDS 4
#define REED_PIN 2

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB+NEO_KHZ800);

WiFiClientSecure espClient;
WiFiManager wm;

PubSubClient mqtt_client(espClient);
String mqtt_client_id = "pcboflight_";


#define R 0
#define G 1
#define B 2
#define BRIGHTNESS 3
#define UPDATED 4
#define IS_ON 5
uint8_t states[6] = {0};

void wifi_setup();
void mqtt_setup();
void mqtt_reconnect();
void mqtt_callback(char *topic, byte *payload, unsigned int len);
void setBrightness(uint8_t value);
void setColor(uint8_t r, uint8_t g, uint8_t b);
void reed_update();

void setup(){
  Serial.begin(9600);
  delay(1000);
  Serial.println("======== PCB Of Light ========");

  espClient.setInsecure();

  wifi_setup();
  mqtt_setup();

  pinMode(REED_PIN, INPUT_PULLUP);
  states[IS_ON] = true;

 strip.begin();
 strip.show();
 strip.setBrightness(50);
}


void loop(){

  // EVERY 100ms
  static unsigned long t = millis();
  if(millis()-t > 100){
    strip.show();

    if(!mqtt_client.connected()){
      mqtt_reconnect();
    }
    t = millis();
  }

  // Light update
  if(states[UPDATED]){
    if(states[IS_ON]){
      setBrightness(states[BRIGHTNESS]);
    }else{
      setBrightness(0);
    }
    setColor(states[R], states[G], states[B]);
    char json[200] = "";
    sprintf(json, "{\"r\":%d, \"g\":%d, \"b\":%d, \"brightness\":%d, \"is_on\": %d}\0", states[R], states[G], states[B], states[BRIGHTNESS], states[IS_ON]);
    mqtt_client.publish("pcboflight/test", json);
    Serial.println("Send");
    states[UPDATED] = 0;
  }
  if(mqtt_client.connected()){
    mqtt_client.loop();  // In the main loop to prevent timeout disconnect
  }

  // General updates
  reed_update();
  wm.process();  // WiFi manager update
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
    if(data.containsKey("r")) states[R] = (uint8_t)data["r"];
    if(data.containsKey("g")) states[G] = (uint8_t)data["g"];
    if(data.containsKey("b")) states[B] = (uint8_t)data["b"];
    if(data.containsKey("brightness")) states[BRIGHTNESS] = (uint8_t)data["brightness"];
    if(data.containsKey("is_on")) states[IS_ON] = (uint8_t)data["is_on"];
    states[UPDATED] = 1;
  }
  return;
}


void setBrightness(uint8_t value){
  strip.setBrightness(value);
  strip.show();
  return;
}

void setColor(uint8_t r, uint8_t g, uint8_t b){
  for(int c=0; c<NUM_LEDS; c++){
    strip.setPixelColor(c, r, g, b);
  }
  strip.show();
  return; 
}

void reed_update(){
  static bool prec_reed = LOW;
  if(digitalRead(REED_PIN) != prec_reed){  // Inverted (pullup) and invertedby design (magnet <-> off)
    if(digitalRead(REED_PIN)){
      states[IS_ON] = true;
      states[UPDATED] = true;
      prec_reed = HIGH;
    } else {
      states[IS_ON] = false;
      states[UPDATED] = true;
      prec_reed = LOW;
    }
  }
  return;
}