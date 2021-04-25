#include "ESP8266WiFi.h"
#include "ESPAsyncWebServer.h"
#include "FS.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <movingAvg.h>
#define AP_SSID "Smart_Agriculture"
#define clk 13
#define Oclk 12
#define data 14
#define OFF_Time_D 38
#define ON_Time_D 18
#define OFF_Time_U 30
#define ON_Time_U 13
movingAvg avgpH(50);
LiquidCrystal_I2C lcd(0x27, 16, 2);
Adafruit_BME280 bme; // I2C
AsyncWebServer server(80);
File file;
unsigned long LCDpreviousMillis = 0;
unsigned long One_Sec_counter = 0;
String SetRec = "Default";
String s = "";
String Wname = "";
String Wpass = "";
String rtemp = "30", rhumi = "40", rPH = "6.7";
String stemp = "25", shumi = "5", day = "12", sPH = "6.7", sA = "0", sB1 = "0", sB2 = "0";
float TH_inTemp;
float TH_inTempH = 0;
float TH_inTempC = 0;
float TH_setTempFromUSER;
boolean TH_on = false;
boolean TH_holdh = false;
boolean TH_holdc = false;
boolean TH_x = false;
bool Connection_State = 0;
bool OfflineMode = 0;
bool ConnectionMode = 1;
bool LCDDisplay = 0;
bool LCDFlipPage = 1;
bool Nut_Added = 0;
bool OUTBIN[8] = { 1, 1, 1, 1, 1, 1, 1, 0 };
bool OUTBIN_1[2] = { 1, 0 };
bool Old_OUTBIN[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
unsigned long Humi_previousMillis;
float HumiSec = 0;
float HumiRead = 0;
float HumiSet = 0;
float volt = 0.00;
float pHSensor = 0.00;
float pH_Sec = 0;
float pH_Read = 7;
float pH_Set = 9;
bool pHLock = 0;
float pHDownLimit = 0.2;
float pHUpLimit = 0.2;
unsigned long pH_previousMillis;
int Current_time_counting = 0;
int ml[] = { 0, 0, 0 };
int sul = 0;
int Nutr_On_Time = 0;
bool Nutr_On_Lock = false;
bool Nutr_Off_Lock = false;
int Nutr_Off_Time = 10;
int val = 1;
unsigned long Nutr_previousMillis = 0;
void LCDprint(String NetName, String ip) {
 if (LCDFlipPage) {
 lcd.clear();
 if (ConnectionMode)
 lcd.print("Network Name:");
 else
 lcd.print("Connected To:");
 lcd.setCursor(0, 1);
 lcd.print(NetName);
 }
 else {
 lcd.clear();
 lcd.print("IP adress is:");
 lcd.setCursor(0, 1);
 lcd.print(ip);
 }
 LCDFlipPage = !LCDFlipPage;
}
void LCDloading() {
 if (ConnectionMode) {
 lcd.print("Loading, Startin");
 lcd.setCursor(0, 1);
 lcd.print("g WiFi Hotspot.");
 }
 else {
 lcd.print("Connecting To...");
 lcd.setCursor(0, 1);
 lcd.print(String(Wname));
 }
}
void Scan() {
 WiFi.softAPdisconnect(true);
 WiFi.mode(WIFI_STA);
 WiFi.disconnect();
 WiFi.scanDelete();
 WiFi.mode(WIFI_AP);
 WiFi.softAP(AP_SSID);
 delay(100);
 Serial.println(WiFi.softAPIP());
 LCDDisplay = 1;
 int np = 0;
 for (int i = 0; i < 10; i++) {
 int n = WiFi.scanNetworks(false, true);
 if (np < n) {
 s = "";
 for (int i = 0; i < n; i++) {
 s = s + "\"" + WiFi.SSID(i) + "\",";
 Serial.println(s);
 }
 np = n;
 s = s.substring(0, (s.length() - 1));
 Serial.println(s);
 }
 }
}
String Read_file(String location) {
 String FileCont;
 file = SPIFFS.open(location, "r");
 while (file.available()) {
 FileCont += char(file.read());
 }
 file.close();
 return FileCont;
}
void Write_file(String location, String message) {
 file = SPIFFS.open(location, "w");
 file.print(message);
 file.close();
}
void Add_to_file(String location, String message) {
 file = SPIFFS.open(location, "a");
 file.print(message);
 file.close();
}
void TE_Cont() {
 if ((TH_inTemp < TH_setTempFromUSER || TH_holdh) && TH_holdc) {
 TH_holdc = true;
 TH_holdh = true;
 if (TH_on) {
 OUTBIN[2] = 0;
 OUTBIN[4] = 1;

 if (TH_x) {
 TH_inTempH = TH_inTemp;
 TH_x = false;
 }
 }
 if (TH_inTemp > TH_setTempFromUSER + 0.2) {
 OUTBIN[0] = 1;
 OUTBIN[4] = 1;
 if (!TH_x) {
 TH_inTempC = TH_inTemp;
 }
 TH_x = true;
 TH_on = false;
 }
 if (TH_inTemp < TH_setTempFromUSER - 1) {
 TH_holdh = false;
 TH_on = true;
 OUTBIN[0] = 0;
 OUTBIN[4] = 1;
 }
 }
 else {
 TH_holdh = false;
 TH_holdc = false;
 if (TH_on) {
 OUTBIN[2] = 1;
 OUTBIN[4] = 0;
 if (TH_x) {
 TH_inTempH = TH_inTemp;
 TH_x = false;
 }
 }
 if (TH_inTemp < TH_setTempFromUSER - 1) {
 TH_on = false;
 OUTBIN[0] = 1;
 OUTBIN[4] = 1;
 if (!TH_x) {
 TH_inTempC = TH_inTemp;
 }
 TH_x = true;
 }
 if (TH_inTemp > TH_setTempFromUSER + 1) {
 TH_holdc = true;
 TH_on = true;
 OUTBIN[0] = 0;
 OUTBIN[4] = 0;
 }
 }
 if (TH_inTemp > TH_setTempFromUSER + 1.5) {
 TH_holdc = false;
 }
 if (TH_inTemp < TH_setTempFromUSER - 1.5) {
 TH_holdc = true;
 TH_holdh = true;
 }
}
void pH_Get() {
 volt = analogRead(A0);
 volt = avgpH.reading(volt);
 pHSensor = -0.0194 * volt + 23.32;
 rPH = String(pHSensor);
}
void pH_Cont() {
 pH_Read = rPH.toFloat();
 pH_Set = sPH.toFloat();
 if ((millis() - pH_previousMillis >= 250)) {
 pH_previousMillis = millis();
 pH_Sec = pH_Sec + 0.25;
 }
 if (pH_Read >= pH_Set - pHUpLimit && pH_Read <= pH_Set + pHDownLimit) {
 OUTBIN[1] = 0;
 OUTBIN[3] = 0;
 pHDownLimit = 0.2;
 pHUpLimit = 0.2;
 }
 else if (pH_Read > pH_Set + pHDownLimit) {
 pHDownLimit = 0;
 pHUpLimit = 0.4;
 if (pH_Sec <= OFF_Time_D) {
 OUTBIN[1] = 0;
 OUTBIN[3] = 0;
 }
 else if (pH_Sec > OFF_Time_D && pH_Sec <= (OFF_Time_D + ON_Time_D)) {
 OUTBIN[1] = 1;
 OUTBIN[3] = 0;
 }
 else {
 pH_Sec = 0;
 }
 }
 else if (pH_Read < pH_Set - pHUpLimit) {
 pHDownLimit = 0.4;
 pHUpLimit = 0;
 if (pH_Sec <= OFF_Time_U) {
 OUTBIN[1] = 0;
 OUTBIN[3] = 0;
 }
 else if (pH_Sec > OFF_Time_U && pH_Sec <= (OFF_Time_U + ON_Time_U)) {
 OUTBIN[1] = 0;
 OUTBIN[3] = 1;
 }
 else {
 pH_Sec = 0;
 }
 }
}
void Light_Cont() {
 Write_file("/Time.txt", String(Current_time_counting));
 if (Current_time_counting <= ((day.toInt()) * 60 * 60)) {
 OUTBIN[7] = 1;
 }
 else {
 OUTBIN[7] = 0;
 }
}
void Humi_Cont() {
 if ((millis() - Humi_previousMillis >= 250)) {
 Humi_previousMillis = millis();
 HumiSec = HumiSec + 0.25;
 }
 if (HumiRead <= (HumiSet)) {
 if (HumiSec <= 8) {
 digitalWrite(16, HIGH);
 }
 else if ((HumiSec > 8 && HumiSec <= (8 + 0.25)) && HumiRead > HumiSet - 1.5) {
 digitalWrite(16, LOW);
 }
 else if (HumiSec > 8 && HumiSec <= (8 + 2)) {
 digitalWrite(16, LOW);
 }
 else {
 HumiSec = 0;
 }
 }
 else {
 digitalWrite(16, LOW);
 }
}
void Output_cont() {
 for (int i = 0; i < sizeof(OUTBIN); i++) {
 if (Old_OUTBIN[i] != OUTBIN[i]) {
 for (int i = 0; i < 8; i++) {
 digitalWrite(data, OUTBIN[i]);
 digitalWrite(clk, 1);
 delay(1);
 digitalWrite(clk, 0);
 delay(1);
 }
 digitalWrite(Oclk, 1);
 delay(1);
 digitalWrite(Oclk, 0);
 for (int j = 0; j < sizeof(OUTBIN); j++) {
 Old_OUTBIN[j] = OUTBIN[j];
 }
 }
 }
}
String offline(int OfflineMode_) {
 if (OfflineMode_ == 1)
 return "/offline.html";
 else
 return "/WiFi.html";
}
void BME_Get() {
 rtemp = String(bme.readTemperature());
 rhumi = String(bme.readHumidity());
 TH_inTemp = rtemp.toFloat();
 TH_setTempFromUSER = stemp.toFloat();
 HumiRead = rhumi.toFloat();
 HumiSet = shumi.toFloat();
}
void Nutri_Cont() {
 if (millis() - Nutr_previousMillis >= 1000) {
 Nutr_previousMillis = millis();
 if (Nutr_On_Lock) {
 Nutr_On_Time = Nutr_On_Time + 1;
 }
 if (Nutr_Off_Lock) {
 Nutr_Off_Time = Nutr_Off_Time - 1;
 }
 OUTBIN[6] = 0;
 }
 val = digitalRead(4);
 if (ml[sul] > 0 && !Nutr_Off_Lock) {
 OUTBIN[6] = 0;
 }
 else {
 OUTBIN[6] = 1;
 if (ml[sul] == 0 && sul <= 2) {
 sul = sul + 1;
 }
 }
 if (val == 0 && !Nutr_Off_Lock) {
 Nutr_On_Lock = true;
 }
 if (Nutr_On_Time >= 10) {
 Nutr_Off_Lock = true;
 OUTBIN[6] = 1;
 }
 if (Nutr_Off_Time <= 0) {
 Nutr_On_Time = 0;
 Nutr_Off_Time = 10;
 Nutr_On_Lock = false;
 Nutr_Off_Lock = false;
 ml[sul] = ml[sul] + 1;
 }
 if (ml[0] == 0 && ml[1] == 0 && ml[2] == 0) {
 pHLock = 1;
 }
}
void setup() {
 pinMode(clk, OUTPUT);
 pinMode(Oclk, OUTPUT);
 pinMode(data, OUTPUT);
 pinMode(16, OUTPUT);
 pinMode(2, OUTPUT);
 pinMode(0, INPUT);
 avgpH.begin();
 Output_cont();
 digitalWrite(16, HIGH);
 Serial.begin(115200);
 lcd.begin();
 if (!SPIFFS.begin()) {
 lcd.clear();
 lcd.print("SPIFFS ERROR");
 return;
 }
 if (!bme.begin(0x76)) {
 lcd.clear();
 lcd.print("BME ERROR");
 }
 server.begin();
 String conf = Read_file("/conf.txt");
 ConnectionMode = conf.substring(conf.indexOf(">") + 1, conf.lastIndexOf(">")).toInt();
 Wname = conf.substring(conf.indexOf("=") + 1, conf.lastIndexOf("="));
 Wpass = conf.substring(conf.indexOf(":") + 1, conf.lastIndexOf(":"));
 Connection_State = conf.substring(conf.indexOf("?") + 1, conf.lastIndexOf("?")).toInt();
 String setrec = Read_file("/setrec.txt");
 stemp = setrec.substring(setrec.indexOf(">") + 1, setrec.lastIndexOf(">"));
 shumi = setrec.substring(setrec.indexOf("=") + 1, setrec.lastIndexOf("="));
 day = setrec.substring(setrec.indexOf(":") + 1, setrec.lastIndexOf(":"));
 sPH = setrec.substring(setrec.indexOf("?") + 1, setrec.lastIndexOf("?"));
 sA = setrec.substring(setrec.indexOf("@") + 1, setrec.lastIndexOf("@"));
 sB1 = setrec.substring(setrec.indexOf("#") + 1, setrec.lastIndexOf("#"));
 sB2 = setrec.substring(setrec.indexOf("$") + 1, setrec.lastIndexOf("$"));
 SetRec = setrec.substring(setrec.indexOf("&") + 1, setrec.lastIndexOf("&"));
 Serial.println(stemp + shumi + day + sPH + SetRec + sA + sB1 + sB2);
 Current_time_counting = (Read_file("/Time.txt")).toInt();
 LCDloading();
 server.on("/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(SPIFFS, "/bootstrap.min.css", String());
 });
 server.on("/jquery.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(SPIFFS, "/jquery.min.js", String());
 });
 server.on("/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(SPIFFS, "/bootstrap.min.js", String());
 });
 server.on("/ResetSetting", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", "window.location.reload();");
 Write_file("/conf.txt", "w>" + String(1) + ">ssid=null=pass:null:cs?" + String(0) + "?");
 delay(1000);
 ESP.restart();
 });
 server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", "rtemp.setValue(" + rtemp + "<=35?" + rtemp + ""
 ":35);rhumi.setValue(" + rhumi + "<=100?" + rhumi + ":100);rPH.setValue(" + rPH + "<=14?"
 "" + rPH + ":14);rtempESP=" + rtemp + ";rhumiESP=" + rhumi + ";rPHESP=" + rPH + ";");
 });
 server.on("/set", HTTP_GET, [](AsyncWebServerRequest * request) {
 for (int i = 0; i < request->args(); i++) {
 if (i == 0) {
 stemp = String(request->arg(i));
 }
 else if (i == 1) {
 shumi = String(request->arg(i));
 }
 else if (i == 2) {
 day = String((String(request->arg(i))).toInt());
 }
 else if (i == 3) {
 sPH = String(request->arg(i));
 }
 else if (i == 4) {
 sA = String(request->arg(i));
 }
 else if (i == 5) {
 sB1 = String(request->arg(i));
 }
 else if (i == 6) {
 sB2 = String(request->arg(i));
 }
 }
 Write_file("/setrec.txt", "stemp>" + stemp + ">shumi=" + shumi + "=day:" + day + ":sPH?"
 "" + sPH + "?A@" + sA + "@B1#" + sB1 + "#B2$" + sB2 + "$" + "SetRec!" + SetRec + "!");
 ml[0] = sA.toInt();
 ml[1] = sB1.toInt();
 ml[2] = sB2.toInt();
 request->send(200, "text/plain", "alert(\"The new setting has been set\")");
 });
 server.on("/recDropdown", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", "rec_dropdown({" + Read_file("/drec.txt") + Read_file(""
 "/urec.txt") + "}, '" + SetRec + "');");
 });
 server.on("/SetRec", HTTP_GET, [](AsyncWebServerRequest * request) {
 for (int i = 0; i < request->args(); i++) {
 if (i == 0) {
 SetRec = String(request->arg(i));
 }
 }
 request->send(200, "text/plain", "sres(\"" + SetRec + "\")");
 });
 server.on("/SendUserRecipe", HTTP_GET, [](AsyncWebServerRequest * request) {
 for (int i = 0; i < request->args(); i++) {
 if (i == 0) {
 Write_file("/urec.txt", String(request->arg(i)));
 }
 }
 request->send(200, "text/plain", "alert(\"Recipe added and saved\")");
 });
 if (ConnectionMode) {
 Scan();
 server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(SPIFFS, offline(OfflineMode), String());
 });
 if (Connection_State == 1) {
 server.on("/GetWiFi", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", "wifi =[" + s + "]; "
 "document.getElementById(\"ermes\").innerHTML = \"Connection failed please try again"
 " or continue using offline mode\";"
 "document.getElementById(\"ermes\").style.color = \"red\";");
 });
 }
 else {
 server.on("/GetWiFi", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", "wifi =[" + s + "];");
 });
 }
 server.on("/setWiFi", HTTP_GET, [](AsyncWebServerRequest * request) {
 for (int i = 0; i < request->args(); i++) {
 if (i == 0) {
 Wname = String(request->arg(i));
 }
 else if (i == 1) {
 Wpass = String(request->arg(i));
 }
 }
 Serial.println(Wname);
 Serial.println(Wpass);
 Write_file("/conf.txt", "w>" + String(0) + ">ssid=" + Wname + "=pass:" + Wpass + ":cs?"
 "" + String(0) + "?");
 if (WiFi.softAPdisconnect(true)) {
 ESP.restart(); //reset.
 }
 });
 server.on("/offline", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", "window.location.reload();");
 OfflineMode = !OfflineMode;
 });
 }
 else {
 WiFi.begin(Wname, Wpass);
 int i = 0;
 while (WiFi.status() != WL_CONNECTED) {
 delay(1000);
 Serial.println("Connecting to WiFi..");
 if (i >= 20) {
 Connection_State = 1;
 Write_file("/conf.txt", "w>" + String(1) + ">ssid=null=pass:null:cs?" + String(1) + "?");
 ESP.restart();
 }
 i++;
 }
 Serial.println(WiFi.localIP());
 LCDDisplay = 1;

 server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(SPIFFS, "/index.html", String());
 });
 server.on("/updatedrec", HTTP_GET, [](AsyncWebServerRequest * request) {
 String drec = "";
 for (int i = 0; i < request->args(); i++) {
 if (i == 0) {
 drec = String(request->arg(i));
 }
 }
 Write_file("/drec.txt", drec);
 });
 }
 server.on("/GetSetSetting", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(200, "text/plain", ""
 "stemp.setValue(" + stemp + ");"
 "Pushedtemp = " + stemp + ";"
 "shumi.setValue(" + shumi + ");"
 "Pushedhumi = " + shumi + ";"
 "sPH.setValue(" + sPH + ");"
 "PushedPH = " + sPH + ";"
 "day.setValue(" + day + ");"
 "Pushedday = " + day + ";"
 "A.setValue(" + sA + ");"
 "PushedAdtemp = " + sA + ";"
 "B1.setValue(" + sB1 + ");"
 "PushedB1 = " + sB1 + ";"
 "B2.setValue(" + sB2 + ");"
 "PushedB2 = " + sB2 + ";"
 "");
 });
 server.on("/Debug", HTTP_GET, [](AsyncWebServerRequest * request) {
 request->send(SPIFFS, "/Debug.html", String());
 });
 server.on("/DebugButtons", HTTP_GET, [](AsyncWebServerRequest * request) {
 for (int i = 0; i < request->args(); i++) {
 if (i == 0) {
 OUTBIN[0] = (String(request->arg(i)) == "true");
 }
 else if (i == 1) {
 OUTBIN[1] = (String(request->arg(i)) == "true");
 }
 else if (i == 2) {
 OUTBIN[2] = (String(request->arg(i)) == "true");
 }
 else if (i == 3) {
 OUTBIN[3] = (String(request->arg(i)) == "true");
 }
 else if (i == 4) {
 OUTBIN[4] = (String(request->arg(i)) == "true");
 }
 else if (i == 5) {
 OUTBIN[5] = (String(request->arg(i)) == "true");
 }
 else if (i == 6) {
 OUTBIN[6] = (String(request->arg(i)) == "true");
 }
 else if (i == 7) {
 OUTBIN[7] = (String(request->arg(i)) == "true");
 }
 else if (i == 8) {
 OUTBIN_1[0] = (String(request->arg(i)) == "true");
 digitalWrite(16, OUTBIN_1[0]);
 }
 else if (i == 9) {
 OUTBIN_1[1] = (String(request->arg(i)) == "true");
 digitalWrite(2, OUTBIN_1[1]);
 }
 }
 request->send(200, "text/plain", "console.log(" + String(OUTBIN[0]) + ", " + String(OUTBIN[1]) + ", " + String(OUTBIN[2]) + ", " + String(OUTBIN[3]) + ", " + String(OUTBIN[4]) + ", " + String(OUTBIN[5]) + ", " + String(OUTBIN[6]) + ", " + String(OUTBIN[7]) + ", " + String(OUTBIN[8]) + ", " + String(OUTBIN[9]) + ");");
 });
}
void loop() {
 if (millis() - LCDpreviousMillis >= 4500) {
 LCDpreviousMillis = millis();
 if (LCDDisplay) {
 if (ConnectionMode == 1)
 LCDprint(String(AP_SSID), WiFi.softAPIP().toString());
 else
 LCDprint(String(Wname), WiFi.localIP().toString());
 }
 Light_Cont();
 }
 if (millis() - One_Sec_counter >= 1000) {
 One_Sec_counter = millis();
 if (Current_time_counting >= 86400)
 Current_time_counting = 0;
 else
 Current_time_counting = Current_time_counting + 1;
 BME_Get();
 pH_Get();
 }
 if (Nut_Added) {
 pH_Cont();
 }
 TE_Cont();
 Humi_Cont();
 Output_cont();
}
