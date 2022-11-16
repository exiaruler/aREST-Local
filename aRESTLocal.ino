/*
  This a simple example of the aREST Library for the ESP8266 WiFi chip.
  See the README file for more details.

  Written in 2015 by Marco Schwartz under a GPL license.
*/

// Import required libraries
#include <ESP8266WiFi.h>
#include <aREST.h>
#include "Servo.h"
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

// Create aREST instance
aREST rest = aREST();

// WiFi parameters
const char* ssid = "veda";
const char* password = "gn002dynames";

// The port to listen for incoming TCP connections
#define LISTEN_PORT           80
// Create an instance of the server
WiFiServer server(LISTEN_PORT);

// Variables to be exposed to the API
// global variable
String ok="OK";
// global operations
boolean uploadMode;
String warning;
String devices;
String queryData;
boolean backgroundRunning;
// global methods 
String servoMove(Servo servo,int pin,int start,int move,int gap,int between,int loop){
  
  if(gap<0){
    gap=1000;
  }
  if(between<0){
    between=5000;
  }
  if(loop<=0){
    loop=1;
  }
  if(start<0){
    return "start position missing";
  }
  servo.attach(pin);
  servo.attach(pin,500,2400);
  if(!servo.attached()){
    return "Servo pin does not exist";
  }
  for(int i=0; i<loop; i++){
  servo.write(start);
  delay(gap);
  servo.write(move);
  delay(between);
  servo.write(start);
  delay(gap);
  }
  servo.detach();
  return ok;
}
// led blink function
int x = 1;
unsigned long d1, d2;
void ledBlink(int pin,long interval){
  d1,d2;
  pinMode(pin, OUTPUT);
  d2=millis();
  if (d2-d1 >= interval){
    x=1-x;
    d1=millis();
    digitalWrite(pin,x);
  }
  d1,d2;
}
// calculate time for mins
int calculateTime(int t){
 int min=1000*60;
 return min*t;
}
// Declare functions to be exposed to the API
// Setting Routes
int uploadModeConfig(String command);
int uploadModeConfig(String command){
  int r=0;
  if(command=="true"){
    uploadMode=true;
    r=1;
  }
  if(command=="false"){
    uploadMode=false;
    r=1;
  }
  return r;
}
int getRoutes(String command);

int ledControl(String command);
int testControl(String command);

//artoria
int saberPush(String command);
int saberStart(String command);

//exia
int exiaStart(String command);
int exiaCycle(String command);
int exiaTransition(String command);
int exiaModeChange(String command);
int exiaChangeState(String command);

// devices
// artoria
Servo saber;
int servo_pin2=D1; //artoria
int pushS=0;
int pushB=180;
String saberState,saberStatus,saberWarning,saberRoutes;
// exia
Servo exia;
String exiaState,exiaWarning;
boolean exiaMode;
String exiaStatus;
String exiaRoutes;
int between=1000;

int ledPin=D2;
String ledState="off";
// query for route data 
int getRoutes(String command){
  int r=0;
  if(command=="saber"){
    queryData=saberRoutes;
    r=1; 
  }
  if(command=="exia"){
    queryData=exiaRoutes;
    r=1;
  }
  if(queryData!=""&r==0){
    queryData="";
  }
  return r;
}
void setup(void)
{
  // Start Serial
  Serial.begin(115200);
  // Init variables and expose them to REST API
  warning="",backgroundRunning=false,devices="Artoria|Exia", uploadMode=false;
  queryData="";
  //saber
  saberState="",saberStatus="",saberWarning="";
  saberRoutes="saberPush|saberStart";
  // exia
  exiaMode=false;
  exiaStatus="",exiaState="",exiaWarning="";
  exiaRoutes="exiaStart|exiaCycle|exiaTransition|exiaModeChange(false,true)|exiaChangeState(startup,normal,pre-trans-am,trans-am,off)";
  // global json
  rest.variable("Devices",&devices);
  // 1 device
  //rest.variable("Warning",&warning);
  rest.variable("Background",&backgroundRunning);
  rest.variable("QueryData",&queryData);
  //devices json
  // saber
  rest.variable("SaberState",&saberState);
  rest.variable("SaberStatus",&saberStatus);
  rest.variable("SaberWarning",&saberWarning);
  // exia 
  rest.variable("ExiaMode",&exiaMode);
  rest.variable("ExiaStatus",&exiaStatus);
  rest.variable("ExiaState",&exiaState);
  rest.variable("ExiaWarning",&exiaWarning);
  
  // Function to be exposed
  // config route
  rest.function("upload",uploadModeConfig);
  rest.function("routes",getRoutes);
  // Saber http route
  rest.function("saberPush",saberPush);
  rest.function("saberStart",saberStart);
  // exia http route

  // test route
  rest.function("led",ledControl);
  rest.function("testControl",testControl);
  // Give name & ID to the device (ID should be 6 characters long)
  rest.set_id("1");
  rest.set_name("aREST Local");

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    pinMode(2,OUTPUT); 
    digitalWrite(2,0); 
    delay(500);
    Serial.print(".");
  }
  //
  // wireless upload
  ArduinoOTA.setPassword((const char *)"123");
   ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    pinMode(2,OUTPUT); 
    for(int a=0; a<3; a++){
      ledBlink(2,900);
    }
    pinMode(2,INPUT); 
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  //
  Serial.println("");
  Serial.println("WiFi connected");
  digitalWrite(2,1);
  pinMode(2,INPUT); 
  // Start the server
  server.begin();
  ArduinoOTA.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  // Reset server when upload mode true
  if(uploadMode==true){
    saberReset();
    pinMode(2,OUTPUT); 
    ledBlink(2,900);
    ArduinoOTA.handle();
  }else{
  // Handle REST calls
  WiFiClient client = server.available();
  // Reset when cannot connect to wifi
  // Turn on D2 on board LED on when down
  if(WiFi.status()!= WL_CONNECTED){
    pinMode(2,OUTPUT); 
    digitalWrite(2,0); 
    saberReset();
  }else{
    digitalWrite(2,1);
    pinMode(2,INPUT); 
  }
  background();
  if (!client) {
    return;
  }
  while(!client.available()){
    delay(1);
  }
  rest.handle(client);
  }
}
// background processes 
void background(){
  if(ledState=="blink"){
    blink();
  }
  if(ledState=="standard"){
    pinMode(ledPin,OUTPUT);
    digitalWrite(ledPin,1);
  }
  if(ledState=="off"){
    digitalWrite(ledPin,0);
    backgroundRunning=false;
    d1,d2;
    x=1;
  }

}
int saberPush(String command){
   warning="";
   int r=0;
   String move;
   if(saberState=="standard"){
      move=servoMove(saber,servo_pin2,180,89,1000,500,1);
     if(move==ok){
       saberState="fade";
       saberStatus=move;
       r=1;
     }
     }else{
       if(saberState=="fade"){
            move=servoMove(saber,servo_pin2,180,89,1000,500,2);
          if(move==ok){
            saberState="standard";
            saberStatus=move;
            r=1;
          }
       }
     }
     if(move!=ok){
       warning=move;
       saberWarning=move;
     }
  return r;
}
int saberStart(String command){
   warning="";
   int r=0;
   String move;
   if(saberState==""){
      move=servoMove(saber,servo_pin2,180,89,1000,500,1);
     if(move==ok){
       saberState="standard";
       saberStatus=move;
       r=1;
     }
  }
     if(move!=ok){
       warning=move;
       saberWarning=move;
     }
  return r;
}
// Reset method use in board when wifi down
void saberReset(){
  warning="";
  String move;
   if(saberState=="standard"){
      move=servoMove(saber,servo_pin2,180,89,100,1000,2);
     if(move==ok){
       saberState="";
     }
     }else{
       if(saberState=="fade"){
            move=servoMove(saber,servo_pin2,180,89,100,1000,1);
          if(move==ok){
            saberState="";
          }
       }
     }
     if(move!=ok){
       warning=move;
     }
}


// Custom function accessible by the API
int ledControl(String command) {

  // Get state from command
  int state = command.toInt();
  pinMode(ledPin,OUTPUT);
  Serial.println("[LEDON]");
  digitalWrite(ledPin,state);
  return 1;
}

int testControl(String command){
  ledState=command;
  backgroundRunning=true;
  return 1;
}
void blink(){
  ledBlink(D2,250);
}
