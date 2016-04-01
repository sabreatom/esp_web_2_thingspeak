/*
 * esp_sensor_web.ino - program to display sensor values using web server
 *
 * Author: Aleksandrs Maklakovs
 *
 * Purpose: Provide sensor's current values for Python script, which sends
 *          them to ThingSpeak.
 *
 * Usage: Run on ESP8266 using Arduino IDE.
 */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "DHT.h"

#define DHTPIN 2        //DHT11 connected to GPIO2
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);
 
const char* ssid = "ssid";    //replace with AP's SSID tp connect to
const char* password = "password"; //replace with AP's password
MDNSResponder mdns;

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  char t_buffer[10];
  char h_buffer[10];

  float h = dht.readHumidity();
  String hum=dtostrf(h,0,0,h_buffer);
  float t = dht.readTemperature();
  String temp=dtostrf(t,0,0,t_buffer);
  
  String sens_message = "Temperature: ";
  sens_message += temp;
  sens_message += "oC, humidity: ";
  sens_message += hum;
  sens_message += "%.";
  
  digitalWrite(led, 1);
  server.send(200, "text/plain", sens_message);
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}
 
void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  
  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void){
  server.handleClient();
} 
