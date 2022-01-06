#include <FS.h>
#include "SPIFFS.h"
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include "SHT2x.h"
#include <Servo.h>

#define Moist_PIN 35
#define Spray_Pump 15
#define Water_Valve 2
#define Servo_PIN 25
#define sensorPower 5
#define blinker 16

char* ssid;//     = "DIGI_08f670";
char* password;// = "aa21f709";

AsyncWebServer server(80);

int temp;
int moist;
int humid;
bool sts;
int minTemp;
int maxTemp;
int minMoist;
int maxMoist;
int minHumid;
int maxHumid;
SHT2x SHT2x;
Servo servo;

void ConfigSensors(){
  File file = SPIFFS.open("/config.json","r");
  String json="";
  DynamicJsonDocument doc (1024);
  while (file.available())
  {
    json+=char(file.read());
  }
  deserializeJson(doc, json);
  minTemp = doc["minTemp"].as<int>();
  maxTemp = doc["maxTemp"].as<int>();
  minHumid = doc["minHumid"].as<int>();
  maxHumid = doc["maxHumid"].as<int>();
  minMoist = doc["minMoist"].as<int>();
  maxMoist = doc["maxMoist"].as<int>();
  Serial.print("minTemp:");
  Serial.println(minTemp);
  Serial.print("maxTemp:");
  Serial.println(maxTemp);
  Serial.print("minHumid:");
  Serial.println(minHumid);
  Serial.print("maxHumid:");
  Serial.println(maxHumid);
  Serial.print("minMoist:");
  Serial.println(minMoist);
  Serial.print("maxMoist:");
  Serial.println(maxMoist);
  //ssid=doc["ssid"];
  //password=doc["password"];
  file.close();
  }

void ConfigWifi(){
  File file = SPIFFS.open("/wifi.json","r");
  String json="";
  DynamicJsonDocument doc (1024);
  while (file.available())
  {  
    json+=char(file.read());
  }
  deserializeJson(doc, json);
  String s = doc["ssid"].as<String>();
  String p = doc["password"].as<String>();
  ssid =const_cast<char*>(s.c_str());
  password =const_cast<char*>(p.c_str());
  Serial.print("Ssid:");
  Serial.println(ssid);
  Serial.print("Pass:");
  Serial.println(password);
  //ssid=doc["ssid"];
  //password=doc["password"];
  file.close();
  ConnectToWifi();
}

void SaveSensorConfig(int a, int b, int c, int d, int e, int f)
{
  File file = SPIFFS.open("/config.json","w");
  DynamicJsonDocument doc(1024);
  doc["minTemp"]=a;
  doc["maxTemp"]=b;
  doc["minHumid"]=c;
  doc["maxHumid"]=d;
  doc["minMoist"]=e;
  doc["maxMoist"]=f;
  serializeJson(doc,file);
  file.close();
}

void SaveWifiConfig(String a, String b)
{  
  File file = SPIFFS.open("/wifi.json","w");
  DynamicJsonDocument doc(1024);
  doc["ssid"]=a;
  doc["password"]=b;
  serializeJson(doc,file);
  file.close();
}

String SendParams()
{  
  return String(minTemp)+ " " + String(maxTemp) + " " + String(minHumid) + " "+ String(maxHumid)+ " " + String(minMoist)+ " " + String(maxMoist);
}

void ConnectToWifi(){
  
  WiFi.begin(ssid, password);
  delay (1000);
  if(WiFi.status()==WL_CONNECTED)
  {
  sts=1;
  }
  else 
  {
    SetAsAP();
  }
}

void SetAsAP(){
  const char* ssidAP="Smart GreenHouse";
  const char* passAP="pass1234";
  WiFi.softAP(ssidAP, passAP);
  Serial.println(WiFi.softAPIP());
  sts=0;
}

void Blink(){
  delay(500);
  digitalWrite(blinker,HIGH);
  delay(100);
  digitalWrite(blinker,LOW);
}

void Mapping(){
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", "text/html");
    });
    server.on("/connect", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/connect.html", "text/html");
    });
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/config.html", "text/html");
    });
    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSHTTemperature().c_str());
     });
    server.on("/moisture", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readMoist().c_str());
    });
    server.on("/TogRel", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", TogRel().c_str());
    });
    server.on("/SendParams", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", SendParams().c_str());
    });
    server.on("/WifiCredentials", HTTP_GET, [](AsyncWebServerRequest *request){
      int paramsNr = request->params();
      Serial.println(paramsNr);
      String NewSsid;
      String NewPass;
      
      for(int i=0;i<paramsNr;i++)
      {
        AsyncWebParameter* p = request->getParam(i);
        if ((p->name())=="ssid" ){
          String str=p->value();
          NewSsid=str;
        }
        else if ((p->name())=="pass")
        {
          String str=p->value();
          NewPass=str;
        }
      }
      SaveWifiConfig(NewSsid,NewPass);
      WiFi.disconnect();
      WiFi.softAPdisconnect (true);
      ConfigWifi();
    request->send_P(200, "text/plain", "Sent");
    });
    server.on("/SaveConfig", HTTP_GET, [](AsyncWebServerRequest *request){
      int paramsNr = request->params();
      Serial.println(paramsNr);
        AsyncWebParameter* p;
        p = request->getParam(0);
        minTemp = p->value().toInt();
        p = request->getParam(1);
        maxTemp = p->value().toInt();
        p = request->getParam(2);
        minHumid = p->value().toInt();
        p = request->getParam(3);
        maxHumid = p->value().toInt();
        p = request->getParam(4);
        minMoist = p->value().toInt();
        p = request->getParam(5);
        maxMoist = p->value().toInt();
       
        SaveSensorConfig(minTemp,maxTemp,minHumid,maxHumid,minMoist,maxMoist);
    
    request->send_P(200, "text/plain", "Sent");
    });
    
    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSHTHumidity().c_str());
    });
  }



String TogRel()
{
   digitalWrite(Spray_Pump, !digitalRead(Spray_Pump));
   delay(1000);
   Serial.println(digitalRead(Spray_Pump));
   return String(digitalRead(Spray_Pump));
}

String readMoist() {
  moist =map(analogRead(Moist_PIN),0,4096,100,0);
  Serial.print("moisture");
  Serial.println(moist);
  return String (moist);
}

String readSHTTemperature() {
  Serial.print("Temperature(%RH): ");
  Serial.println(SHT2x.GetTemperature(),1);
  temp=SHT2x.GetTemperature(),1;
  return String(temp);
}

String readSHTHumidity() {
  Serial.print("Humidity(C): ");
  Serial.println(SHT2x.GetHumidity(),1);
  humid = SHT2x.GetHumidity(),1;
  return String(humid);
}

void actions(){
    if (moist<minMoist)
    {
      digitalWrite(Water_Valve, HIGH);
    } 
    else if (moist>maxMoist)
    {
      digitalWrite(Water_Valve, LOW);  
    }
    if (temp<minTemp){
      servo.write(0);
    }
    else if (temp>maxTemp){
      servo.write(90);
    }
  }

  void setup() {
  pinMode(blinker,OUTPUT);
  pinMode(Moist_PIN,INPUT);
  pinMode(Spray_Pump,OUTPUT);
  pinMode (Water_Valve,OUTPUT);
  digitalWrite(Water_Valve,LOW);
  digitalWrite(Spray_Pump,HIGH); 
  servo.attach(Servo_PIN);
  servo.write(0);
  if(!SPIFFS.begin())
  {
    Serial.println("error");
    return;
  }
  ConfigWifi();
  ConfigSensors();
  Serial.println(WiFi.localIP()); 
  Serial.println(sts);
  // mapping pages and actions
  Mapping();
  server.begin();
  Wire.begin(5,22);
  SHT2x.begin();
  Serial.begin(115200);
}

void loop() 
{
  readSHTHumidity();
  readSHTTemperature();
  readMoist();
  actions();
  delay(2000);
  if(sts==0)
  {
    Blink();
  }
}
