/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
//#include <DNSServer.h>

#define measures 600
#define measure_delay 100
#define hd_address 0

int sensor = A0;
volatile int frequency;
float flow_measure[measures];
float total_flow = 0;
float flow_output;
float calib_constant = 1.00f;
float calib_received;
unsigned long t_start;
unsigned long t_end;
String pub_answer;
char pub_answer_char[16];
char chip_id[8];


/************************* WiFi Access Point *********************************/

//#define WLAN_SSID       "YourWifiSSID"
//#define WLAN_PASS       "YourWifiPassword"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "m16.cloudmqtt.com"
#define AIO_SERVERPORT  14336                  // use 8883 for SSL
#define AIO_USERNAME    "YourUserName"
#define AIO_KEY         "YourPassword"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish brew_flow = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/readings/100");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe calibration = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/calibration/100");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

 
void rpm(){
    frequency++;
  }


void setup() {
  Serial.begin(115200);
  delay(10);

 String(ESP.getChipId()).toCharArray(chip_id,8);


 WiFiManager wifiManager; 
 wifiManager.setConfigPortalTimeout(180);


if(!wifiManager.autoConnect(chip_id,"brew")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 
  //if you get here you have connected to the WiFi
  Serial.println("Wifi connected!");

// Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&calibration);

// configuracoes de leitura do sensor com resistor pull-down
  digitalWrite(sensor, HIGH);
  attachInterrupt(0,rpm,RISING);
  sei();

  // inicializar as leituras
  for (int i=0;i<measures;i++){
    flow_measure[i] = 0;
  }

EEPROM.begin(115200);
if (EEPROM.read(hd_address)!=NULL){
  Serial.print("\n Recuperando valor da constante de calibracao. \n");
  EEPROM.get(hd_address,calib_constant);
  EEPROM.end();
}

}


void loop() {
 
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop

  total_flow = 0;
  t_start = millis();
  
  for (int i=0;i<measures;i++){
    
    flow_measure[i] = (float)frequency/11;
    frequency = 0;
    total_flow = total_flow + flow_measure[i];
    
    if (i == measures-1){
      break;
    }
    else{
      delay(measure_delay);
    }
  }

  flow_output = total_flow/((float)measures);
  t_end = millis();

  Serial.print(F("\nFluxo de cerveja: "));
  Serial.print(flow_output);
  Serial.print(" L/min ");


 pub_answer = String(flow_output,3) + String(" : ") + String(t_end-t_start);
 pub_answer.toCharArray(pub_answer_char, 16);
  
  if (! brew_flow.publish( pub_answer_char )) 
    {
    ESP.reset();
  } 
  else {
   //Serial.println(F("OK!"));
  }
    

// subscribe
  Adafruit_MQTT_Subscribe *subscription;
  while(subscription = mqtt.readSubscription(100)){
    
    if (subscription == &calibration) {
 
      calib_received = atof((char*)calibration.lastread);
 
    }}

// stores data to EEPROM
if (calib_received!=NULL){
    EEPROM.begin(115200);
    EEPROM.put(hd_address,calib_received);  
    EEPROM.get(hd_address,calib_constant);
    EEPROM.end();
}
  

  
  Serial.print(String("\nConstante de calibracao: ")+calib_constant);

  // Serial.print(String("frequency: ")+frequency);
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
