#include <atomic>

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <SparkFun_TB6612.h>

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

#include "NewPing.h"


const char* ssid = "xyz"; // Wifi name
const char* password = "123"; // Wifi password


// Custom UUIDs for Robot Control Service
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define DEVICE_MANUFACTURER "Mobiler"
#define DEVICE_NAME "RobotA"

// Standard UUIds for Device Information
#define DEVINFO_UUID (uint16_t)0x180a
#define DEVINFO_MANUFACTURER_UUID (uint16_t)0x2a29
#define DEVINFO_NAME_UUID (uint16_t)0x2a24
#define DEVINFO_SERIAL_UUID (uint16_t)0x2a25


// Left Motor
const int Motor_1_Pin_1 = 15;
const int Motor_1_Pin_2 = 27;
const int Enable_1_Pin = 14;

// Right Motor
const int Motor_2_Pin_1 = 25;
const int Motor_2_Pin_2 = 26;
const int Enable_2_Pin = 4;

// Motor definitions
Motor* motor1;
Motor* motor2;

// Relay 
const int Relay_1 = 18;
const int Relay_2 = 19;


// Ultra sensor pins- single pin configuration
const int triggercenter = 32;
const int echocenter = 32;
const int triggerright = 21;
const int echoright = 21;
const int triggerleft = 33;
const int echoleft = 33;

boolean leftFlag;
boolean centerFlag;
boolean rightFlag;

#define TRIGGER_DIRECTION_DISTANCE 30

#define MAX_DISTANCE 200 // Maximum distance (in cm) to ping.
#define PING_INTERVAL 40 // Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo).

//boolean volatile started=false;

std::atomic<bool> started(false);

#define ONBOARD_LED  13 // only for Adafruit Huzzah32


// Setting PWM properties
#define FREQUENCY 30000
#define pwmChannel1  0
#define pwmChannel2  1
#define resolution  8

#define dutyCycle_7 100
#define dutyCycle_11 90

int speed;

// For Aysnc
AsyncWebServer server(80);

// For sonar
NewPing sonarC(triggercenter, echocenter,MAX_DISTANCE);
NewPing sonarL(triggerleft, echoleft,MAX_DISTANCE);
NewPing sonarR(triggerright, echoright,MAX_DISTANCE);

void keepLow();

void deviateRight();
void deviateLeft();
void forward();
void reverse(boolean shouldDelay);

void turnOn();
void turnOff();

void handleEchoSensor();

void wifi();

boolean checkObstacle(float distance);


class MyserverCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    keepLow();

    //Serial.println("Connected");
  }

  void onDisconnect(BLEServer *pServer)
  {
    keepLow();
    //Serial.println("Disconnected");
    delay(200);
    pServer->getAdvertising()->start();
  }
};

// Call back class to handle bluetooth data events
class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string value = pCharacteristic->getValue();
      //Done to remove NUL character coming from MIT App
      value.erase(find(value.begin(),value.end(),'\0'),value.end());
   
      // if(value.compare("start7")==0)
      // {
      //   speed=dutyCycle_7;
      //   started=true;
      //   forward();
      // } 
      if(value.compare("start")==0)
      {
        speed=dutyCycle_11;
        started=true;
        forward();
      }
      else if(value.compare("stop")==0)
      {
        // This should be synchronized to avoid race condition with echo
        started=false;
        keepLow();
        Serial.println("Stopped");
      }
      else if(value.compare("reverse")==0)
      {
       reverse(false);
      }else if(value.compare("right")==0){
        deviateRight();
      }else if(value.compare("left")==0){
        deviateLeft();
      }
      else if(value.compare("on")==0){
        turnOn();
      }
      else if(value.compare("off")==0){
        turnOff();
      }
      
  }
};

void setup()
{
  Serial.begin(9600);
  wifi();
  pinMode(ONBOARD_LED,OUTPUT);
  pinMode(Relay_1, OUTPUT);
  pinMode(Relay_2, OUTPUT);
  turnOff();
  // PIN mode will be handled by the New ping library
  /* pinMode(triggerleft,OUTPUT);
  pinMode(echoleft,INPUT);

  pinMode(triggercenter,OUTPUT);
  pinMode(echocenter,INPUT);

  pinMode(triggerright,OUTPUT);
  pinMode(echoright,INPUT); */
 
  // Pin mode of Motor pins internally handled by the library
  motor1=new Motor(Motor_1_Pin_1, Motor_1_Pin_2, Enable_1_Pin, 1,pwmChannel1,FREQUENCY,resolution);
  motor2=new Motor(Motor_2_Pin_1, Motor_2_Pin_2, Enable_2_Pin, 1,pwmChannel2,FREQUENCY,resolution);

  digitalWrite(ONBOARD_LED,HIGH);
  keepLow();
  
  BLEDevice::init(DEVICE_NAME);

  // Main Service
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyserverCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor((new BLE2902()));

  pCharacteristic->setValue("Welcome to Robot Control");
  pService->start();

  // Device Info Service. Use Standard UUIds

  BLEService *pDeviceInfoService = pServer->createService(DEVINFO_UUID);
  BLECharacteristic *manCharacteristic = pDeviceInfoService->createCharacteristic(DEVINFO_MANUFACTURER_UUID, BLECharacteristic::PROPERTY_READ);
  manCharacteristic->setValue(DEVICE_MANUFACTURER);

  BLECharacteristic *deviceNameCharacteristic = pDeviceInfoService->createCharacteristic(DEVINFO_NAME_UUID, BLECharacteristic::PROPERTY_READ);
  deviceNameCharacteristic->setValue(DEVICE_NAME);

  BLECharacteristic *deviceSerialCharacteristic = pDeviceInfoService->createCharacteristic(DEVINFO_SERIAL_UUID, BLECharacteristic::PROPERTY_READ);
  String chipId = String((uint32_t)(ESP.getEfuseMac() >> 24), HEX);
  deviceSerialCharacteristic->setValue(chipId.c_str());
  pDeviceInfoService->start();

  BLEAdvertisementData adv;
  adv.setName(DEVICE_NAME);
  adv.setCompleteServices(BLEUUID(SERVICE_UUID));

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->addServiceUUID(DEVINFO_UUID);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setAdvertisementData(adv);
  pServer->getAdvertising()->start();

  
}

void loop()
{
  if(started.load()){
   handleEchoSensor();
  }
  //delay(500)
  
}

void wifi(){
  WiFi.mode(WIFI_STA);
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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");

}

 void keepLow(){
    switchOff(*motor1,*motor2);
} 

void forward()
{
  motor1->drive(speed);
  motor2->drive(speed);
  Serial.println("Moving forward");
}

void reverse(boolean shouldDelay)
{
  motor1->drive(-speed);
  motor2->drive(-speed);
  //Serial.println("Moving backward");
  if(shouldDelay){
    delay(300);
  }
  
}

void deviateRight()
{
  reverse(true);
  right(*motor1,*motor2,speed);
  //Serial.println("Turning right");
  delay(750);
  brake(*motor1,*motor2);
  delay(200);
  forward();
}

void deviateLeft()
{
  reverse(true);
  left(*motor1,*motor2,speed);
  //Serial.println("Turning left");
  delay(750);
  brake(*motor1,*motor2);
  delay(200);
  forward();
}

void handleEchoSensor(){
  // Send ping, get distance in cm
	float distanceL = sonarL.ping_cm();
  delay(PING_INTERVAL);
  float distanceC = sonarC.ping_cm();
  delay(PING_INTERVAL);
  float distanceR = sonarR.ping_cm();
  delay(PING_INTERVAL);

 /*  Serial.println("Distances: ");
  Serial.println(distanceL);
  Serial.println(distanceC);
  Serial.println(distanceR); */
  //delay(1000);

  leftFlag=checkObstacle(distanceL);
  centerFlag=checkObstacle(distanceC);
  rightFlag=checkObstacle(distanceR);
  
  if ((leftFlag) || (rightFlag) || (centerFlag))
  {

    if (centerFlag)
    {
      if (leftFlag)
      {
        deviateRight();
      }

      else if (rightFlag)
        deviateLeft();
      else
        deviateLeft();
    }
    else if (leftFlag)
    {
      deviateRight();
    }
    else if (rightFlag)
    {
      deviateLeft();
    }
    else
    {
      deviateLeft();
    }
  }

 else
  {
    forward();
  }
}

boolean checkObstacle(float distance){
  if(distance==0 || distance >TRIGGER_DIRECTION_DISTANCE){
    return false;
  } else{
    return true;
  }

}

void turnOn(){
  digitalWrite(Relay_1, LOW);
  digitalWrite(Relay_2, LOW);
  Serial.println("Current flowing");
}
void turnOff(){
  digitalWrite(Relay_1, HIGH);
  digitalWrite(Relay_2, HIGH);
  Serial.println("Current not flowing");
}
