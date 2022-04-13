//Sesnor to determine liquid left in tank
//Requires values for full and empty tank before use
//Built by # info @ https://autobud.io

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <StringArray.h>
#include <AsyncElegantOTA.h>
#include <HTTPClient.h>



//#define SensorPin A0


//int stoph20calibrate = 0;
//END PUMP

//Global Vars
const char* WiFiSSID = "WiFiSSID";
const char* WiFiPassword = "WiFiPassword";
const char* ssidd = "Autobud";
const char* passwordd = "Grow4life";

unsigned long lastTime = 0;

//Fires every day
unsigned long timerDelay = 1800000;
//unsigned long timerDelay = 30000;

const char* PARAM_FULL = "FullInt";
const char* PARAM_EMPTY = "EmptyInt";
const char* PARAM_VOLUME = "volume";
const char* PARAM_ET = "ET";
const char* PARAM_FT = "FT";


float sensorValue = 0.0;
float DifCalc = 0.0;
float TankLevel = 0.0;
float Volume = 0.0;
float amt = 0.0;
int TakeData = 0;
int EmptyValue = 0;
int FullValue = 0;

String mac = "";

//Vars for Tank Volume Measurement
const int trigPin = 5;  
const int echoPin = 18;  
long duration;
long cm;
long inches;
int distance = 0;
bool wifisetup = false;
int localonly = 0;
String ID = "001";
const char* serverName = "http://192.168.0.2:1880/tank";

AsyncWebServer server(80);

//NEW HTML
const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE HTML><html><head>
  <title>Tank Level</title>

<style type="text/css">
            h2 {
                font-family: courier;
                font-size: 20pt;
                color: black;
                border-bottom: 2px solid blue;
            }
            p {
                font-family: arial, verdana, sans-serif;
                font-size: 12pt;
                color: grey;
            }
            .red_txt {
                color: red;
            }
        </style>
  
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <h2>Tank Level</h2>
  <p><i>V1.0</i></p>
  <script>
    function submitMessage() {
      alert("Value Saved");
      setTimeout(function(){ document.location.reload(false); }, 500);   
    }
  </script></head><body>
    <p><b>Control</b></p>
    <button onclick="UpdateNow();">Update Now</button>
    </form><br>

  <p><b>Current Data</b></p>
   </form>
   Tank Level: %TANK%
   </form><br>
    </form><br>
   Fluid Remaining: %amt% Gallons
   </form><br>
   Device MAC: %mac%

   <p><b>Configuration</b></p>
    </form><br>
    <form action="/get" target="hidden-form">
    Empty Tank Distance ( %ET% Inches): <input type="number " name="ET">
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><br>
      <form action="/get" target="hidden-form">
    Full Tank Distance ( %FT% Inches): <input type="number " name="FT">
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><br>
   <button onclick="CalibrateTank();">Calibrate Water Tank</button>
   </form><br>
    <form action="/get" target="hidden-form">
    Water Tank Volume ( %volume% Gallons): <input type="number " name="volume">
    <input type="submit" value="Submit" onclick="submitMessage()">
    </form><br>
   

   <p><b>Connections</b></p>
        <form action="/get" target="hidden-form">
         WiFi SSID (%WiFiSSID%): <input type="text" name="WiFiSSID">
         <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
        <form action="/get" target="hidden-form">
         WiFi Password (****): <input type="text" name="WiFiPassword">
         <input type="submit" value="Submit" onclick="submitMessage()">
        </form><br>
   
    
  </div>

</body>
<script>
  function UpdateNow() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/UpdateNow", true);
    phr.send();
  }

function CalibrateFullTank() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/CalibrateFullTank", true);
    phr.send();
  }

function CalibrateEmptyTank() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/CalibrateEmptyTank", true);
    phr.send();
  }

function CalibrateTank() {
    var phr = new XMLHttpRequest();
    phr.open('GET', "/CalibrateTank", true);
    phr.send();
  }
            
  
 </script> 
 </html>)rawliteral";


 //SET UP SPIFFS
String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

String processor(const String& var){
    if(var == "TANK"){
    return String(TankLevel);
  }
 else if(var == "volume"){
    return readFile(SPIFFS, "/volume.txt");
  }
   else if(var == "level"){
    return readFile(SPIFFS, "/level.txt");
  }
  else if(var == "amt"){
    return String(amt);
  }
  else if(var == "ET"){
    return readFile(SPIFFS, "/EmptyInt.txt");
  }
   else if(var == "FT"){
    return readFile(SPIFFS, "/FullInt.txt");
  }         
 else if(var == "mac"){
    return String(mac);
  }
  else if(var == "WiFiSSID"){
    return readFile(SPIFFS, "/ssid.txt");
  }
  else if(var == "WiFiPassword"){
    return readFile(SPIFFS, "/wifipasswd.txt");
  }
  
  return String();
}


void setup() {
  Serial.begin(115200);

if(!SPIFFS.begin()){
    Serial.println("An Error has occurred while mounting SPIFFS");  
    
    
    ESP.restart();
  }
  else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }
// Fire up pins
pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
pinMode(echoPin, INPUT); // Sets the echoPin as an Input   

//wifi
Serial.print(wifisetup);
//// pull saved value from SPIFFs
String savedssid = readFile(SPIFFS, "/ssid.txt");
String savedpass = readFile(SPIFFS, "/wifipasswd.txt");
int ssidlength = savedssid.length();
Serial.print("SSID Length:");
Serial.print(ssidlength);


//if the SSID is blank, then revert to local mode
if (ssidlength == 0) {
  wifisetup == true;
  localWifi();
}


 //If Wifi config was pressed, skip this, if not, run:
 if (wifisetup == false) {
  // String savedssid = readFile(SPIFFS, "/ssid.txt");
 //  String savedpass = readFile(SPIFFS, "/wifipasswd.txt");
   WiFi.mode(WIFI_STA);
   Serial.print("Using SSID ");
   Serial.println(savedssid);
   Serial.print("Using Password ");
   Serial.println(savedpass);
   //char hname[19];
   //snprintf(hname, 12, "ESP%d-LIGHT", 32);
   char hname[] = "vufovufo000b";
   WiFi.begin(savedssid.c_str(), savedpass.c_str());
   WiFi.setHostname(hname);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    wifisetup == true;
    localWifi();
    //return;
  }

  Serial.println();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
 }
//END wifi 


//Load HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });



  
  server.on("/UpdateNow", HTTP_GET, [](AsyncWebServerRequest * request) {
   Serial.print("Pulling readings");
   TakeData = 1;

   
    request->send_P(200, "text/plain", "Post");
  });
  

   server.on("/CalibrateTank", HTTP_GET, [](AsyncWebServerRequest * request) {
   Serial.print("Calibrating Tank from readings");
   CalibrateTank();
    request->send_P(200, "text/plain", "Calibrating");
  }); 
    
AsyncElegantOTA.begin(&server); 


//NEW Dealing with UI Vars
server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;

   if (request->hasParam(PARAM_VOLUME)) {
      inputMessage = request->getParam(PARAM_VOLUME)->value();
      writeFile(SPIFFS, "/volume.txt", inputMessage.c_str());
    }

    else if (request->hasParam(WiFiSSID)) {
      inputMessage = request->getParam(WiFiSSID)->value();
      writeFile(SPIFFS, "/ssid.txt", inputMessage.c_str());
    }
     else if (request->hasParam(WiFiPassword)) {
      inputMessage = request->getParam(WiFiPassword)->value();
      writeFile(SPIFFS, "/wifipasswd.txt", inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_FT)) {
      inputMessage = request->getParam(PARAM_FT)->value();
      writeFile(SPIFFS, "/FullInt.txt", inputMessage.c_str());
    }
      else if (request->hasParam(PARAM_ET)) {
      inputMessage = request->getParam(PARAM_ET)->value();
      writeFile(SPIFFS, "/EmptyInt.txt", inputMessage.c_str());
    }
    
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage);
});
//server.onNotFound(notFound);
server.begin(); 
mac = (WiFi.macAddress());
}

void loop() {

  
  //Send an HTTP POST request every X minutes
  if (((millis() - lastTime) > timerDelay) or (TakeData == 1)) {
    Serial.println("Taking Measurements and Posting Data");
    //Check WiFi connection status
   // if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      
      // Pull data:
      DistanceCalc();
      PullTankLevel();

       float FullTankCalc = readFile(SPIFFS, "/FullInt.txt").toFloat();
       float EmptyTankCalc = readFile(SPIFFS, "/EmptyInt.txt").toFloat();

     //Check for junk data before posting 
     if (inches > EmptyTankCalc and < FullTankCalc)
     {
   

      // Your Domain name with URL path or IP address with path
      http.begin(serverName);

      // Specify content-type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      // Data to send with HTTP POST
      String httpRequestData = "Id=" + ID + "&msg=" + TankLevel + "&Amt=" + amt;          
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
        
      // Free resources
      http.end();
   // }
    //else {
    //  Serial.println("WiFi Disconnected");
    //}
    lastTime = millis();
    TakeData = 0;
     }
  }
}


// new global vars
//void POSTDATA( void )
//{
//  HTTPClient http;
//       
//      // Your Domain name with URL path or IP address with path
//      http.begin(serverName);
//
//      // Specify content-type header
//      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
//      // Data to send with HTTP POST
//      String httpRequestData = "Id=" + ID + "&msg=" + sensorValue + "&Temp=" + temp + "&Hum=" + hum;          
//      // Send HTTP POST request
//      int httpResponseCode = http.POST(httpRequestData);
//
//      Serial.println(httpRequestData);
//      Serial.print("HTTP Response code: ");
//      Serial.println(httpResponseCode);
//        
//      // Free resources
//      http.end();
//}

void DistanceCalc( void)
{
       // Clears the trigPin
//digitalWrite(trigPin, LOW);
//delayMicroseconds(2);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
 
  // Convert the time into a distance
  cm = (duration/2) / 29.1;    
  inches = (duration/2) / 74;


  
  
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  
  delay(250);;

}


void CalibrateTank( void )
{
int Dif = 0;  
int EmptyTank = readFile(SPIFFS, "/EmptyInt.txt").toInt();
int FullTank = readFile(SPIFFS, "/FullInt.txt").toInt();
Serial.println("Calibrating Tank");
  Dif = (EmptyTank - FullTank);
  String DifSTR = String(Dif);
  Serial.println(Dif);
//Write Dif to spiffs
 writeFile(SPIFFS, "/Tankdif.txt", DifSTR.c_str());
}

void PullTankLevel( void )
{
       //float FullTankCalc = readFile(SPIFFS, "/FullInt.txt").toFloat();
       float EmptyTankCalc = readFile(SPIFFS, "/EmptyInt.txt").toFloat();
       float DifCalc = readFile(SPIFFS, "/Tankdif.txt").toFloat();
       float RawLevel;
       RawLevel = ((EmptyTankCalc - inches) / DifCalc);
       TankLevel = (RawLevel)*100;
       //Calculate reaming fluid
       float remvol = readFile(SPIFFS, "/volume.txt").toFloat();;
       amt = remvol * RawLevel;

}

void localWifi ( void ){
    Serial.println("Local WiFi");
    WiFi.softAP(ssidd, passwordd) ;
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    server.begin();
    localonly = 1;
    wifisetup = true;
}
