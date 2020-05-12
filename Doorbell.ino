
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoOTA.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>

#include "wifi.h"

//from https://github.com/arcao/Syslog
#include "Syslog.h"

/* Contains
  const char* wifiSsid     = "...."; //SSID of your Wi-Fi router
  const char* wifiPassword = "....""; //Password of your Wi-Fi router
*/

//each 60 sec, do a ping call
#define HEARTBEAT_TIMEOUT 60000

//no replay during 8 sec following a key pressed
#define BUTTON_PRESSED_TIMEOUT 8000

#define SYSLOG_SERVER "192.168.2.242"
#define SYSLOG_PORT 514
#define DEVICE_HOSTNAME "doorbell"
#define APP_NAME "-"


//FIXME: avoid hardcoded
//FIXME: prefer to use dns ?
const char* notifyUrl    = "http://nuc.lan:3000/api/bose/ALL/notify";
const char* heartbeatUrl = "http://nuc.lan:3000/ping"; //heartbeat


int buttonState = LOW;
boolean sending = false;

unsigned long sendingTime = 0;
unsigned long lastHeartbeat = 0;
unsigned long current = 0;

const char *hostname = DEVICE_HOSTNAME;

char chipId[10];

WiFiUDP udpClient;
Syslog syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN);

// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin( 115200);
  delay( 100);

  sprintf( chipId, "%08X", ESP.getChipId()); 
  syslog.appName( chipId);

  Serial.println("\nBooting...");

  Serial.println("Connecting wifi...");

  WiFi.hostname( hostname);
  WiFi.mode(WIFI_STA); //no need AP here
  delay(10);

  WiFi.begin( wifiSsid, wifiPassword);  
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Wifi failed");
    delay(20000);
    ESP.restart();
  }


  Serial.println( WiFi.localIP());
  syslog.log( LOG_INFO, "Booting with ip " + WiFi.localIP().toString()); 
  
  /*
	Because OTA will start mDNS

  if (!MDNS.begin( hostname)) {
    Serial.println("Error setting up MDNS responder!");
    delay(10000); // wait 20 sec
    ESP.restart(); // restart
  }
  Serial.println("mDNS responder started");
  */

  // Port defaults to 8266
  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname( (const char *)hostname); // will call directly MDNS

  // No authentication by default
  ArduinoOTA.setPassword( otaPassword);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
    syslog.log( LOG_INFO, "OTA: updating " + type);
  });
  
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    syslog.log( LOG_INFO, "OTA: done");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    syslog.logf( LOG_INFO, "OTA: error: %u", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });


  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D8, INPUT);

  ArduinoOTA.begin();
  Serial.println("OTA & MDNS Started");


  Serial.println("ready!");
  syslog.log( LOG_INFO, "system ready !");

  //MDNS.notifyAPChange();

}

void notify() {

    HTTPClient httpNotify;  //Declare an object of class HTTPClient
    httpNotify.begin( notifyUrl);

    int httpCode   = httpNotify.GET();
    if (httpCode != HTTP_CODE_OK) { 
        syslog.logf( LOG_ERR, "door button pressed, notification to %s failed: %d", notifyUrl, httpCode);
    }
    else {
	String payload = httpNotify.getString();   //Get the request response payload
	Serial.println(payload);             //Print the response payload
	syslog.log( LOG_INFO, "door button pressed, notification sucessfully sent");
    }

    httpNotify.end();
}

// the loop function runs over and over again forever
void loop() {

  ArduinoOTA.handle();
  //MDNS.update();

  buttonState = digitalRead( D8);
  current = millis();

  if (( current - lastHeartbeat ) >= HEARTBEAT_TIMEOUT) {
    lastHeartbeat = current;
    Serial.println("sending heartbeat");

    //syslog.log( LOG_INFO, "sending heartbeat");

    HTTPClient httpHeartbeat;  //Declare an object of class HTTPClient
    httpHeartbeat.begin( heartbeatUrl);
    httpHeartbeat.GET();
    httpHeartbeat.end();
  }

  
  if ( sending) {

    if ((current - sendingTime) >= BUTTON_PRESSED_TIMEOUT) {
      sending = false;
      digitalWrite(LED_BUILTIN, HIGH); // led off
    }
 
  }

  //on button pushed or each min, process a temp read
  if ( (buttonState == HIGH) && !sending) {

    sendingTime = millis();
    sending = true;
      
    digitalWrite(LED_BUILTIN, LOW); // led on

    Serial.println("doorbutton pressed"); 

    notify();

    
  }

  yield(); // FIXME: is it required ??

}
