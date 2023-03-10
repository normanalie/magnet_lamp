#include <PubSubClient.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Adafruit_NeoPixel.h>

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

void setup(){
  Serial.begin(9600);
  delay(1000);
  Serial.println("======== PCB Of Light ========");

  espClient.setInsecure();

  wifi_setup();
  mqtt_setup();

 strip.begin();
 strip.show();
 strip.setBrightness(255);
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
  Serial.print("[MQTT] - Message on [");
  Serial.print(topic);
  Serial.println("]");
  Serial.println((char)payload[0]);
  if((char)payload[0] == '1'){
    strip.setPixelColor(0, 255, 200, 70);
    strip.setPixelColor(1, 255, 200, 70);
  }else{
    strip.setPixelColor(0, 0, 0, 0);
    strip.setPixelColor(1, 0, 0, 0);
  }
  return;
}