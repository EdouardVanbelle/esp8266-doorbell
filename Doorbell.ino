
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h> 

#include "wifi.h"
/* Contains
//SSID of your network
char ssid[] = "...."; //SSID of your Wi-Fi router
char pass[] = "....""; //Password of your Wi-Fi router
*/

//Fixme avoid hardcoded
char web_hook[] = "http://192.168.2.168:3000/api/bose/ALL/notify";


float h = 0;
float t = 0;
unsigned int count = 0;
unsigned int tooHigh = 0;
char lastUpdate[20];

HTTPClient http;  //Declare an object of class HTTPClient



// the setup function runs once when you press reset or power the board
void setup() {

  Serial.begin( 9600);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D8, INPUT);
  
  digitalWrite(LED_BUILTIN, LOW); //led on during init

  Serial.println("connecting wifi...");

  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  if (!MDNS.begin("wemos")) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  Serial.println("mDNS responder started");

  Serial.println( WiFi.localIP());

  //prepare request 
  http.begin( web_hook);  

  digitalWrite(LED_BUILTIN, HIGH); //stop led

}



int buttonState = LOW;
unsigned int sending = 0;

// the loop function runs over and over again forever
void loop() {

  buttonState = digitalRead( D8);

  if( sending > 0) {
    digitalWrite(LED_BUILTIN, LOW); // led on
    sending--;
  }
  else {
    digitalWrite(LED_BUILTIN, HIGH); // led off
  }
    
  //on button pushed or each min, process a temp read
  if( buttonState == HIGH) {

    //avoid multiple sending
    if( !sending) {
      
      Serial.println("hit"); //reset counter

      int httpCode = http.GET();
      if (httpCode > 0) { //Check the returning code
        String payload = http.getString();   //Get the request response payload
        Serial.println(payload);             //Print the response payload
      }
      else {
        Serial.println("oops"); 
      }
      http.end();
      
    }

    sending = 10;
    
    
    
  }
  count++;


  delay( 100);

}
