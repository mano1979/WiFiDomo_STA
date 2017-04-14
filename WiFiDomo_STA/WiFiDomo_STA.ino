/*
Copyright 2016-2017 Mano Biletsky & Martijn van Leeuwen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software 
and associated documentation files (the "Software"), to deal in the Software without restriction, 
including without limitation the rights to use, copy, modify, merge, publish, and to permit 
persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or 
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, 
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Written by: Mano Biletsky
*/

#include <EEPROM.h>

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiClient.h>
#include <ESP8266mDNS.h>

//needed for library
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

//Enter your Module name. Give every module another name!
char host[15] = "wifidomo";

const int NR_LEN = 4;
char savedNr[NR_LEN] = "";

// The FLASH button on the WiFiDomo module also functions as an ERASE for saved wifi credentials.
// Pushing the button once while in normal mode erases the saved network and starts the Config-Portal.
#define TRIGGER_PIN 0 // Do not change! (unless you really want to... )

int Mode = 0;
const int REDPIN = 4;   // Do not change!
const int GREENPIN = 5; // Do not change!
const int BLUEPIN = 12; // Do not change!
const int SENSORPIN = 13; // Do not change!
const int MODULEPIN = 14;
int sensing = 0;
int ms = 0;
int s = 0;
int m = 0;
int colorMemR = 1023;
int colorMemG = 900;
int colorMemB = 0;
int currentColor[3] = {1023, 1023, 1023};
int targetColor[3] = {1023, 1023, 1023};
const int colorPin[3] = {REDPIN, GREENPIN, BLUEPIN};

void adjustColor() {
  int i;
  int changed = 0;
  do {
    changed = 0;
    for (i = 0; i < 3; i++) {
      if (currentColor[i] != targetColor[i]) {
        int dir = currentColor[i] > targetColor[i] ? -1 : +1;
        currentColor[i] += dir;
        analogWrite(colorPin[i], 1023-currentColor[i]);    
        changed = 1;
      }
      delay(1);
    }
  } while(changed != 0);
}

ESP8266WebServer webServer(80);

String webpage = "<!DOCTYPE html><html><head><title>RGB control</title><meta name='mobile-web-app-capable' content='yes' />"
                 "<meta name='viewport' content='width=device-width' /></head><body style='margin: 0px; padding: 0px;'>"
                 "<canvas id='colorspace'></canvas></body>"
                 "<script type='text/javascript'>"
                 "(function () {"
                 " var canvas = document.getElementById('colorspace');"
                 " var ctx = canvas.getContext('2d');"
                 " function drawCanvas() {"
                 " var colours = ctx.createLinearGradient(0, 0, ctx.canvas.width*0.88, 0);"
                 " for(var i=0; i <= 360; i+=10) {"
                 " colours.addColorStop(i/360, 'hsl(' + i + ', 100%, 50%)');"
                 " }"
                 " ctx.fillStyle = colours;"
                 " ctx.fillRect(0, 0, ctx.canvas.width*0.88, ctx.canvas.height);"
                 " var luminance = ctx.createLinearGradient(0, 0, 0, ctx.canvas.height);"
                 " luminance.addColorStop(0, '#ffffff');"
                 " luminance.addColorStop(0.10, '#ffffff');"
                 " luminance.addColorStop(0.5, 'rgba(0,0,0,0)');"
                 " luminance.addColorStop(0.90, '#000000');"
                 " luminance.addColorStop(1, '#000000');"
                 " ctx.fillStyle = luminance;"
                 " ctx.fillRect(0, 0, ctx.canvas.width*0.88, ctx.canvas.height);"
                 " var greyscale = ctx.createLinearGradient(0, 0, 0, ctx.canvas.height);"
                 " greyscale.addColorStop(0, '#ffffff');"
                 " greyscale.addColorStop(0.10, '#ffffff');"
                 " greyscale.addColorStop(0.90, '#000000');"
                 " greyscale.addColorStop(1, '#000000');"
                 " ctx.fillStyle = greyscale;"
                 " ctx.fillRect(ctx.canvas.width*0.88, 0, ctx.canvas.width*0.12, ctx.canvas.height);"
                 " }"
                 " var eventLocked = false;"
                 " function handleEvent(clientX, clientY) {"
                 " if(eventLocked) {"
                 " return;"
                 " }"
                 " function colourCorrect(v) {"
                 " return Math.round(1023-(v*v)/64);"
                 " }"
                 " var data = ctx.getImageData(clientX, clientY, 1, 1).data;"
                 " var params = ["
                 " 'r=' + colourCorrect(data[0]),"
                 " 'g=' + colourCorrect(data[1]),"
                 " 'b=' + colourCorrect(data[2])"
                 " ].join('&');"
                 " var req = new XMLHttpRequest();"
                 " req.open('POST', '?' + params, true);"
                 " req.send();"
                 " eventLocked = true;"
                 " req.onreadystatechange = function() {"
                 " if(req.readyState == 4) {"
                 " eventLocked = false;"
                 " }"
                 " }"
                 " }"
                 " canvas.addEventListener('click', function(event) {"
                 " handleEvent(event.clientX, event.clientY, true);"
                 " }, false);"
                 " canvas.addEventListener('touchmove', function(event){"
                 " handleEvent(event.touches[0].clientX, event.touches[0].clientY);"
                 "}, false);"
                 " function resizeCanvas() {"
                 " canvas.width = window.innerWidth;"
                 " canvas.height = window.innerHeight;"
                 " drawCanvas();"
                 " }"
                 " window.addEventListener('resize', resizeCanvas, false);"
                 " resizeCanvas();"
                 " drawCanvas();"
                 " document.ontouchmove = function(e) {e.preventDefault()};"
                 " })();"
                 "</script></html>";

void getTargets() {
  for (int i = 0; i < 3; i++) {
    String val = webServer.arg(i);
    if (val != "") {
      targetColor[i] = val.toInt() & 1023;
    }
  }
}

void handleRoot() {
  Serial.println("handle root..");
  int i;
  getTargets();
  adjustColor();
  if ((currentColor[0] == 1023) and (currentColor[1] == 1023) and (currentColor[2] == 1023)){
    sensing = 0;
    ms = 0;
    s = 0;
    m = 0;
  }
  if ((currentColor[0] != 1023) or (currentColor[1] != 1023) or (currentColor[2] != 1023)){
    colorMemR = currentColor[0];
    colorMemG = currentColor[1];
    colorMemB = currentColor[2];
    sensing = 1;
  }
  webServer.send(200, "text/html", webpage); 
}

void handleStatus() {
  Serial.println("Handle Status..");
  String result = ('[' + String(currentColor[0]) + ':' + String(currentColor[1]) +':'+ String(currentColor[2]) + ']');
  webServer.send(200, "text/html", result);
}

void handleSwitch() {
  int i;
  Serial.println("Handle Switch..");
  getTargets();
  webServer.send(200, "text/html");
  adjustColor();
  if ((currentColor[0] == 1023) and (currentColor[1] == 1023) and (currentColor[2] == 1023)){
    sensing = 0;
    ms = 0;
    s = 0;
    m = 0;
  }
  if ((currentColor[0] != 1023) or (currentColor[1] != 1023) or (currentColor[2] != 1023)){
    colorMemR = currentColor[0];
    colorMemG = currentColor[1];
    colorMemB = currentColor[2];
    sensing = 1;
  }
}

//Quick-set function thanks to Klaas van Gend (kaa-ching)
void handleSet(){ 
  Serial.println("handle Set..");
  getTargets();
  for (int i = 0; i < 3; i++) {
    currentColor[i] = targetColor[i];
    analogWrite(colorPin[i], 1023-currentColor[i]);
  }
  sensing = 0; // Turning off the sensor in the quick-set mode
  handleStatus();
}

void sensorTriggered(){
  Serial.println("triggered!");
  //if ((currentColor[0] == 0) and (currentColor[1] == 0) and (currentColor[2] == 0)){ //Not sure if this works or not!
  targetColor[0] = colorMemR;
  targetColor[1] = colorMemG;
  targetColor[2] = colorMemB;
  ms = 0;
  s = 0;
  m = 0;
  adjustColor();
}

void timeToOff(){
  targetColor[0] = 1023;
  targetColor[1] = 1023;
  targetColor[2] = 1023;
  adjustColor();
}

void fade(int pin) {
  for (int u = 0; u < 1024; u++) {
    analogWrite(pin, u);
    delay(1);
  }
  for (int u = 0; u < 1024; u++) {
    analogWrite(pin, 1023 - u);
    delay(1);
  }
}

void testRGB() {

  analogWrite(REDPIN, 0); // R off
  analogWrite(GREENPIN, 0); // G off
  analogWrite(BLUEPIN, 0); // B off
  fade(REDPIN); // R
  fade(GREENPIN); // G
  fade(BLUEPIN); // B
}

void setup() {
  webServer.stop();
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("\n Starting");
  EEPROM.begin(4);
  int i;
  for (i = 0; i < NR_LEN; i++)
     savedNr[i] = EEPROM.read(i);

  pinMode(TRIGGER_PIN, INPUT);
  pinMode(SENSORPIN, INPUT);
  pinMode(MODULEPIN, INPUT);
}

void checkAndSaveNr(const char *newNr) {
  if (strncmp(newNr, savedNr, NR_LEN) != 0) {
    int i;
    for (i = 0; i < NR_LEN; i++) {
      EEPROM.write(i, newNr[i]);
      if (newNr[i] == 0)
        break;
    }
    EEPROM.commit();
  }
  strncpy(savedNr, newNr, NR_LEN);
}

 WiFiManagerParameter custom_host_nr("host_nr", "Asign a (unique) # to this module", "", 10, " disabled");
  WiFiManagerParameter custom_host_nr_choice("<select style=\"width:100%; height:35px\" name='host_nr'>"
  "<option value='1'>1</option>"
  "<option value='2'>2</option>"
  "<option value='3'>3</option>"
  "<option value='4'>4</option>"
  "<option value='5'>5</option>"
  "<option value='6'>6</option>"
  "<option value='7'>7</option>"
  "<option value='8'>8</option>"
  "<option value='9'>9</option>"
  "<option value='10'>10</option>"
"</select>");
    WiFiManager wifiManager;

void loop() {
  // is configuration portal requested?
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    Mode = 0;
    //WiFiManager
    //reset settings
    wifiManager.resetSettings();
    wifiManager.addParameter(&custom_host_nr);
    wifiManager.addParameter(&custom_host_nr_choice);
    analogWrite(REDPIN, 1023);
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.startConfigPortal("WiFiDomo")) {
      Serial.println("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
    }
    checkAndSaveNr(custom_host_nr.getValue());
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
    Mode = 0;
  }
  // Main code
  if(Mode == 0){
    pinMode(REDPIN, OUTPUT);
    pinMode(GREENPIN, OUTPUT);
    pinMode(BLUEPIN, OUTPUT);
    analogWrite(REDPIN, 0);
    analogWrite(GREENPIN, 0);
    analogWrite(BLUEPIN, 0);
    delay(1000);
    if(WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi Connect Failed! Starting Acces Point...");
      delay(1000);
      Mode = 0;
      analogWrite(REDPIN, 1023);
      wifiManager.addParameter(&custom_host_nr);
      wifiManager.addParameter(&custom_host_nr_choice);

      if (!wifiManager.startConfigPortal("WiFiDomo")) {
        Serial.println("failed to connect and hit timeout");
        delay(3000);
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(5000);
      }
      checkAndSaveNr(custom_host_nr.getValue());
    }
    strncat(host, savedNr, NR_LEN);
    Serial.println(String("HAI--") +  host + "--HAI");
    MDNS.begin(host);
    MDNS.addService("http", "tcp", 53);
    webServer.on("/", handleRoot);
    webServer.on("/status", handleStatus);
    webServer.on("/switch", handleSwitch);
    webServer.on("/set", handleSet);
    webServer.begin();
    testRGB();
    Mode = 1;
    MDNS.update();
  }
  if(Mode == 1){
    if(digitalRead(MODULEPIN) == HIGH){ //If module is active and installed then...
      sensing = 0;
    }
    if(sensing == 1){
      if(digitalRead(SENSORPIN)==HIGH){ 
        sensorTriggered();
        delay(1000); //debounce
      }
      ms=ms+1;
      if(ms == 1000){
        s=s+1;
        ms = 0;
      }
      if(s == 60){
        m=m+1;
        s = 0;
        
      }
      if(m == 15){
        timeToOff();
        m = 0;
      }
      Serial.print(m);
      Serial.print(":");
      Serial.print(s);
      Serial.print(":");
      Serial.print(ms);
      Serial.print("\n");
    }
  }
  webServer.handleClient();
  MDNS.update();
}
