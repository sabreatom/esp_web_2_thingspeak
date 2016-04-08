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
#include <DHT.h>
#include "FS.h"

#define DHTPIN 2        //DHT11 connected to GPIO2
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);

//Default SSID name and password:
const char* ssid = "ssid";    //replace with AP's SSID tp connect to
const char* password = "password"; //replace with AP's password

//Buffer for SSID's name and password:
char ssid_buf[20];
char ssid_psw_buf[20];

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

  //Mount file system:
  Serial.println("Mounting FS:");
  SPIFFS.begin();

  //Begin WiFi connection:
  if (fs_rd_ssid(&ssid_buf[0], &ssid_psw_buf[0]) == 1)
  {
    Serial.println("SSID default parameters:");
    Serial.println(ssid);
    Serial.println(password);
    WiFi.begin(ssid, password);
  }
  else
  {
    Serial.println("SSID config parameters:");
    Serial.println(&ssid_buf[0]);
    Serial.println(&ssid_psw_buf[0]);
    WiFi.begin(&ssid_buf[0], &ssid_psw_buf[0]);
  }

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

//-------------------------------------------------------
//Function: write AP's SSID and password to ESP's FS
//Args: SSID and AP's password
//Return: 0 - write done successfully
//        1 - write failed
//-------------------------------------------------------

int fs_wr_ssid(char *ssid_ptr, char *psw_ptr)
{
  File f = SPIFFS.open("/ssid_info.txt", "w");

  if (!f) 
  {
      Serial.println("File creation or modification failed");
      return 1;
  }
  
  f.println(ssid_ptr);
  f.println(psw_ptr);

  f.close();
  return 0;
}

//-------------------------------------------------------
//Function: read AP's SSID and password from FS
//Args: pointer to SSID's name array and pointer to password's array
//Return: 0 - read done successfully
//        1 - read failed
//-------------------------------------------------------

int fs_rd_ssid(char *ssid_ptr, char *psw_ptr)
{
  char tmp;
  Serial.println("Starting to read config file.");
  File f = SPIFFS.open("/ssid_info.txt", "r");
  Serial.println("Config file opened!");
  if (!f) 
  {
      Serial.println("File doesn't exist!");
      return 1;
  }

  //Read SSID's name:
  tmp = (char)f.read();
  while (tmp != '\n')
  {
    if (tmp != '\r')  //to remove carrier return char
    {
      *ssid_ptr = tmp;
      ssid_ptr++;
    }
    else
    {
      Serial.println("SSID carrier return!");
    }
    tmp = (char)f.read();
  }
  *ssid_ptr = '\0';

  //Read SSID's password:
  tmp = (char)f.read();
  while (tmp != '\n')
  {
    if (tmp != '\r')  //to remove carrier return char
    {
      *psw_ptr = tmp;
      psw_ptr++;
    }
    else
    {
      Serial.println("Password carrier return!");
    }
    tmp = (char)f.read();
  }
  *psw_ptr = '\0';

  f.close();
  return 0;
}

//-------------------------------------------------------
 
void loop(void){
  char tmp_char;
  char ssid_buf[20];
  char psw_buf[20];
  unsigned int i = 0;
  
  server.handleClient();
  
  if (Serial.available() > 0) 
  {
    tmp_char = (char)Serial.read();

    if (tmp_char == 's')
    {
      while(1)
      {
        if (Serial.available() > 0) 
        {
          tmp_char = (char)Serial.read();

          if (tmp_char == '\n')
          {
            ssid_buf[i] = '\0';
            i = 0;
            break;
          }
          else
          {
            ssid_buf[i] = tmp_char;
            i++;
          }
        }
      }
      while(1)
      {
        if (Serial.available() > 0) 
        {
          tmp_char = (char)Serial.read();

          if (tmp_char == '\n')
          {
            psw_buf[i] = '\0';
            i = 0;
            break;
          }
          else
          {
            psw_buf[i] = tmp_char;
            i++;
          }
        }
      }
    }

    if(fs_wr_ssid(&ssid_buf[0], &psw_buf[0]) == 0)
    {
      Serial.println("AP's info write was successful:");
      Serial.println(&ssid_buf[0]);
      Serial.println(&psw_buf[0]);
    }
    else
    {
      Serial.println("AP's info write failed");
    }
  }
} 
