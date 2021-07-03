//FS: 3MB OTA: 512kB

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <OakOLED.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define trigsens 16//Trigger IR Sensor
#define staplesens 12//Klammern-vorschub Sensor im Heftkopf (ist "LOW" wenn Klammern voll)
#define possens 13//Positionssensor im Heftkopf
#define cartsens 14//Heftklammern-eingesetzt Sensor im Heftkopf (ist "LOW" wenn Heftklammern eingesetzt sind)
#define EN 15//um Motortreiber L293N zu aktivieren
#define MC23 5//Kanal 2 u. 3 an L293N
#define MC14 4//Kanal 1 u. 4 an L293N
#define adc A0//VCC messen 10k PullDown 330k PullUp 

#define displayinterval 5000//Zeit in ms zwischen Display-Output
#define stapletimeout 500//Zeit in ms fuer heften
#define triggercount 14//Anzahl der registrierten Trigger impulse (Warnsignal)

unsigned long displaymillis = 0;
bool trig;
bool staple;
bool pos;
bool cart;
int triggertimer=0;
int stapletimer=0;//Timer für Zeit zum heften
double voltage;

OakOLED oled;

const char* SSID = "iPhone von Robert Neumann";
const char* PSK = "8094579880";
//const char* PSK = "74858342257742483927";

const char* Hostname;//Wert in dem der Name gespeichert wird
String Hostnamestring = "HEFTKOPF-";//erster Teil des Hostnames (NODE-193B8F)
String macadress;


// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(trigsens, INPUT);
  pinMode(staplesens, INPUT);
  pinMode(possens, INPUT);
  pinMode(cartsens, INPUT);
  pinMode(adc,INPUT);
  pinMode(EN, OUTPUT);
  pinMode(MC23, OUTPUT);
  pinMode(MC14, OUTPUT);
  digitalWrite(EN,LOW);
  digitalWrite(MC23,LOW);
  digitalWrite(MC14,LOW);
  

  Serial.begin(115200);

  Wire.begin(2,0);
  oled.begin();
  oled.clearDisplay();
  
  setup_wifi();

  ArduinoOTA.setHostname(Hostname);
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
  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
// the loop function runs over and over again forever
void loop() {

 ArduinoOTA.handle();
 
 trig=digitalRead(trigsens);
 staple=digitalRead(staplesens);
 pos=digitalRead(possens);
 cart=digitalRead(cartsens);

 voltage = ((analogRead(adc) / 33.0) * 1.07);//* (5.0 / 1023.0)*4.1

 Serial.println(voltage);

 if (trig==1)
  {
    while (trig==1)
     {
      triggertimer++;
      motorbeep();
      delay(25);
      motorstop();
      delay(25);
      
    if(triggertimer>=triggercount)//wenn der Trigger mehrfach hintereinander ausgelöst wurde wird geheftet
      {
        triggertimer=0;
        stapling();
      }
     
      trig=digitalRead(trigsens);
     }

     triggertimer=0;//wenn der Trigger unterbrochen wird geht er auf 0 zurück
   }
/*
 if(trig==1)
 {
  motorforward();
  delay(250);
  motorstop();
  motorbeep();
  //delay(1000);
  //motorreverse();
  //delay(1000);
 }
 else
 {
  motorstop();
 }
*/
  oled.clearDisplay();
  oled.setTextSize(2);
  oled.setTextColor(1);
  oled.setCursor(0,0);//horizontal,vertikal
  oled.print("trig:");
  oled.print(trig);
  oled.setCursor(0,15);//horizontal,vertikal
  oled.print("staple:");
  oled.print(staple);
  oled.setCursor(0,30);//horizontal,vertikal
  oled.print("pos:");
  oled.print(pos);
  oled.setCursor(0,45);//horizontal,vertikal
  oled.print("cart: ");
  oled.print(cart);
  oled.setCursor(68,30);//horizontal,vertikal
  oled.print(voltage);
  oled.display();
}

void stapling()
{
  motorforward();
  
  pos = digitalRead(possens);
 
 while(pos==0)//aus Startposition bewegen
  {
    pos = digitalRead(possens);
    delay(1);
    stapletimer++;

    if(stapletimer>=stapletimeout)
     {
      motorstop();
     }
  }
 while(pos==1)//aus Heftposition bewegen (heften)
  {
    pos = digitalRead(possens);
    delay(1);
    stapletimer++;

    if(stapletimer>=stapletimeout)
     {
      motorstop();
     }
  }
  
  motorstop();
  
  stapletimer=0;
  delay(2000);
}

void motorforward()
{
  digitalWrite(EN,HIGH);
  delay(15);
  digitalWrite(MC14,LOW);
  digitalWrite(MC23,HIGH);
  delay(15);
}

void motorreverse()
{
  digitalWrite(EN,HIGH);
  delay(15);
  digitalWrite(MC23,LOW);
  digitalWrite(MC14,HIGH);
  delay(15);
}

void motorstop()
{
  delay(15);
  digitalWrite(MC23,LOW);
  digitalWrite(MC14,LOW);
  delay(15);
  digitalWrite(EN,LOW);
}

void motorbeep()
{
  digitalWrite(EN,HIGH);
  delay(15);
  digitalWrite(MC14,LOW);
  analogWrite(MC23,45);
  delay(15);
}

void setup_wifi() {
  
  macadress = WiFi.macAddress();
  //Hostnamen erstsellen
  Hostnamestring = Hostnamestring + macadress.charAt(9) + macadress.charAt(10)+ macadress.charAt(12) + macadress.charAt(13) + macadress.charAt(15) + macadress.charAt(16);

  Hostname = Hostnamestring.c_str();//Hostname von String in const char umwandeln (wegen OTA)
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PSK);
  WiFi.hostname(Hostname);

  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
 
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    oled.clearDisplay();
    oled.setTextSize(2);
    oled.setTextColor(1);
    oled.setCursor(0,10);//horizontal,vertikal
    oled.print("verbunden mit:");
    oled.print(SSID);
    oled.display();
    delay(1000);
}
