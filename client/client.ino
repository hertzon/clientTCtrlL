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
#include <ArduinoJson.h>

/************************* WiFi Access Point *********************************/

//#define WLAN_SSID       "NELSON"
//#define WLAN_PASS       "nelson1234567"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "138.197.20.62"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    "nelson"
#define AIO_KEY         "!irfz44n"



/************ Global State (you don't need to change this!) ******************/
char WLAN_SSID[20]="NELSON";
char WLAN_PASS[20]="nelson1234567";
String ID="TRA000002X";
int rele=12;
int greenLed=13;
int button=0;
const char *ssid = "ControlLights";
const char *password = "12345678";
WiFiServer  server(8889);
StaticJsonBuffer<200> jsonBuffer;


String rx;
boolean alreadyConnected = false;
//WiFiClient client;
int prescaler=0;
//String serial="SM-32650001";
String inputString = "";
String command="";
String str_ssid="";
String str_passwd="";
String ssidSubStr="";

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/photocell");
//Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/TRA000002X/estado");
Adafruit_MQTT_Publish photocell = Adafruit_MQTT_Publish(&mqtt,"/TRA000002X/estado");
Adafruit_MQTT_Publish fx = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "test");

// Setup a feed called 'onoff' for subscribing to changes.
//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");
//String strSubs="/"+ID+"/rele";

//const char rele[] PROGMEM = AIO_USERNAME "/TRA000001X/rele";

//Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/rele");
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt,"/TRA000002X/rele");


/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void led(int x){
  if (x==1){
    digitalWrite(greenLed,LOW); 
  }
  if (x==0){
    digitalWrite(greenLed,HIGH);  
  }
}

void relay(int x){
  if (x==0){
    digitalWrite(rele,LOW); 
  }
  if (x==1){
    digitalWrite(rele,HIGH);  
  }  
}

boolean readRelay(){
  if (digitalRead(rele)){
    return true;  
  }else {
    return false;  
  }
}

JsonObject& root = jsonBuffer.createObject();
long sequence=0;

void setup() {
  Serial.begin(115200);
  delay(10);
  pinMode(rele,OUTPUT);
  pinMode(greenLed,OUTPUT);
  pinMode(button,INPUT);
  relay(0);led(1);
  EEPROM.begin(512);

  //Serial.println(F("Adafruit MQTT demo"));
  Serial.println("Wifi Control Light id: "+ID);
  Serial.println("Designed by Ing.Nelson Rodriguez");
  readEeprom();
  conectar();
  Serial.println();
  relay(1);led(1);

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());
  delay(3000);
  

  // Setup MQTT subscription for onoff feed.
  Serial.println("Subscribiendo..");
  mqtt.subscribe(&onoffbutton);
  relay(1);led(1); 
  if (mqtt.connect()){
    Serial.println("Mqtt conectado...desconectando...");
    mqtt.disconnect();
    delay(3000);
  }else {
    Serial.println("ok Mqtt desconectado"); 
  }
  
  
}

uint32_t x=0;
//*************************************************************************************************
void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  if (!digitalRead(button)){
    Serial.println("Btn pressed");
    conectar();
  }

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(3000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
      if (strcmp((char *)onoffbutton.lastread, "1") == 0) {
        relay(1);led(1);break;
      }else if (strcmp((char *)onoffbutton.lastread, "0") == 0) {
        relay(0);led(0);break;
      }
    }
  }

  // Now we can publish stuff!
  //Serial.print(F("\nSending photocell val "));
  //Serial.print(x);
  //Serial.print("...");
  //if (! photocell.publish(x++)) {
  
  String  jsonObj;
  long rssi = WiFi.RSSI();
  root["id"]=ID;
  root["state"] = readRelay();
  root["sequence"] = ++sequence;
  root["rssi"] = rssi;
  long quality=2 * (rssi + 100);
  root["quality"] = quality;
  
  
  root.prettyPrintTo(jsonObj);
  Serial.println(jsonObj);
  //if (! photocell.publish(readRelay()+"hol")) {
  char feed[100];
  jsonObj.toCharArray(feed,jsonObj.length()+1);
  if (! photocell.publish(feed)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

//  if (fx.publish("hola soy esp8266")){
//    Serial.println("ok");
//  }else {
//    Serial.println("ups algo fallo");
//  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  
//  if(! mqtt.ping()) {    
//    Serial.println("Ping server failed!");
//    mqtt.disconnect();
//  }else {
//    Serial.println("Ping server ok!");
//  }
  
}
//*************************************************************************************************
// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;
  Serial.println("MQTT_connect");
  // Stop if already connected.
  if (mqtt.connected()) {
    Serial.println("mqtt esta connect!");
    return;
  }else {
    Serial.print("Connecting to MQTT... ");  
  }

  

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

void readEeprom(){
  Serial.println("Read EEPROM");  
  int i=0;
  char x;
  for (i=0;i<50;i++){
    x=(char)EEPROM.read(i);
    Serial.print(x);
    client.print(x);      
  }
  Serial.println("Fin Read EEPROM");
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void saveSSID(String ssid){
  int i=0;
  Serial.println("Saving SSID...");
  for (i=0;i<20;i++){
    EEPROM.write(i, 0);  
  }
  EEPROM.commit();
  for (i=0;i<ssid.length();i++){
    EEPROM.write(i, ssid.charAt(i)); 
  }  
  delay(50);
  EEPROM.commit();
}
void savePASSWD(String passw){
  int i=0;
  Serial.println("Saving PASSWD...");
  for (i=20;i<49;i++){
    EEPROM.write(i, 0);  
  }
  EEPROM.commit();
  for (i=0;i<passw.length();i++){
    EEPROM.write(i+20, passw.charAt(i)); 
  }  
  delay(50);
  EEPROM.commit();
}

void clearEeprom(){
  Serial.println("Clear EEPROM...");
  int i=0;
  for (i=0;i<50;i++){
    //EEPROM.write(addr, val);
    EEPROM.write(i, 0);  
  }
  delay(50);
  EEPROM.commit();
  Serial.println("Reading EEPROM...");
  for (i=0;i<50;i++){
    //EEPROM.write(addr, val);
    Serial.println(EEPROM.read(i));
  }
}

void readssid(){
  Serial.println("Read SSID...");
  int i=0;
  char x;
  memset(WLAN_SSID,0,sizeof(WLAN_SSID));
  for (i=0;i<20;i++){   
    x=(char)EEPROM.read(i);
    if (isAscii(x) && x!=0){
      Serial.print(x);  
      client.print(x);
      WLAN_SSID[i]=x;
    }      
  }
  client.println("");
  Serial.println("");
}

void readpass(){
  Serial.println("Read PASS...");
  int i=0;
  char x;
  memset(WLAN_PASS,0,sizeof(WLAN_PASS));
  for (i=20;i<49;i++){   
    x=(char)EEPROM.read(i);
    if (isAscii(x) && x!=0){
      Serial.print(x);  
      client.print(x);
      WLAN_PASS[i-20]=x;
    }       
  }  
  client.println("");
  Serial.println("");
}

void conectar(){
  Serial.println("Funcion Conectar....");
  conectar:
  readssid();
  readpass();

  Serial.print("WIFI STATUS: ");Serial.println(WiFi.status());
  switch (WiFi.status()){
    case 0:
      Serial.println("WL_IDLE_STATUS");
    break;
    case 1:
      Serial.println("WL_NO_SSID_AVAIL");
    break;
    case 2:
      Serial.println("WL_SCAN_COMPLETED");
    break;
    case 3:
      Serial.println("WL_CONNECTED");
    break;
    case 4:
      Serial.println("WL_CONNECT_FAILED");
    break;
    case 5:
      Serial.println("WL_CONNECTION_LOST");
    break;
    case 6:
      Serial.println("WL_DISCONNECTED");
    break;
  }

  if (WiFi.status()==WL_DISCONNECTED || WiFi.status()==WL_IDLE_STATUS){
    // Connect to WiFi access point.
    Serial.println(); Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WLAN_SSID);
    Serial.print("Passwd:");
    Serial.println(WLAN_PASS); 
  
    Serial.println("Configure module WIFI_STA");
    WiFi.mode(WIFI_STA);
    WiFi.begin(WLAN_SSID, WLAN_PASS);  
  }

  int timeoutButton=0;  
  int pr=0;
  while (WiFi.status() != WL_CONNECTED) {
    
    delay(100);
    if (++pr>5){
      pr=0;
      Serial.print(".");
    }
    
    digitalWrite(greenLed,!digitalRead(greenLed));
    if (!digitalRead(button)){
      WiFi.disconnect();
      ++timeoutButton;
      Serial.println(timeoutButton);
      if (timeoutButton>15){
        led(1);
        while(!digitalRead(button)){
          Serial.println("Esperando a que se suelte button");
          delay(500);
        }
        Serial.println("Entrando a modo programacion...");
        Serial.print("Configuring access point...");
        delay(3000);
        timeoutButton=0;     
        Serial.println("Configure module WIFI_AP");   
        WiFi.mode(WIFI_AP);
        WiFi.softAP(ssid, password);
        WiFi.printDiag(Serial);
        IPAddress myIP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(myIP);
        server.begin();
        printWifiStatus();   
        while(1){
          yield();
          if (client.status()==CLOSED){
            client.stop();
            if (++prescaler>100000){
              prescaler=0;
              Serial.print("\nConnection closed on client");  
              inputString="";
            }
            
            alreadyConnected = false;     
          }
          if (client) {
            if (!alreadyConnected) {
              // clead out the input buffer:
              client.flush();    
              Serial.println("We have a new client");
              client.println("im controlLights: "+ID); 
              alreadyConnected = true;
            } 
        
            if (client.available() > 0) {
              // read the bytes incoming from the client:
              char thisChar = client.read();
               
              if (thisChar!='\r'){
                inputString += thisChar; 
              }else {
                command=inputString;
                Serial.println("Comando recibido: "+command);
                
                if (command=="alive?"){
                  client.println("alive Ok");
                }
        
                if (command=="serial?"){
                  client.println(ID);  
                }
        
                if (command=="readeprom?"){
                  readEeprom();                  
                }
                if (command=="readessid?"){
                  readssid();                  
                }
                if (command=="readepass?"){
                  readpass();                  
                }
                if (command=="connect!"){
                  Serial.println("Reseting wifi module!!!");
                  WiFi.disconnect();
                  //ESP.reset();
                  //ESP.restart();
                  goto conectar;                  
                }
        
                Serial.print("command length: ");
                Serial.println(command.length());
                if (command.length()>4){
                  ssidSubStr=command.substring(0,4);
                  //Serial.println("ssidSubStr: "+ssidSubStr); 
                  if (ssidSubStr=="Ssid"){
                    str_ssid=command.substring(5,command.length());
                    Serial.println(str_ssid);
                    client.println(str_ssid);
                    saveSSID(str_ssid);
                  }
                  ssidSubStr="";
                  ssidSubStr=command.substring(0,4);
                  if (ssidSubStr=="Pass"){
                    str_passwd=command.substring(5,command.length());
                    Serial.println(str_passwd);
                    client.println(str_passwd);
                    savePASSWD(str_passwd);
                  }
                }
                client.flush();
                command="";inputString="";ssidSubStr="";
              }
              // echo the bytes back to the client:
              //client.print(thisChar);
              // echo the bytes to the server as well:
              //Serial.write(thisChar);
            }
          }else {
            client = server.available();
            if (client.status()==CLOSED){
              client.stop();
              //Serial.print("\nConnection closed on client: "  );
        
              //Serial.println();
              alreadyConnected = false;  
            }
          }
          delay(50);
          if (!digitalRead(button)){
            Serial.println("Saliendo modo programacion");
            break;
          }
          
        }
      }
    }else {
      timeoutButton=0;
    }
  }
  Serial.println("Saliendo de conectar");
}


