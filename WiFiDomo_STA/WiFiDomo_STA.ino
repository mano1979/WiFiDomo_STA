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

void handleRoot() {
  Serial.println("handle root..");
  int i;
  for (i = 0; i < 3; i++) {
    String val = webServer.arg(i);
    if (val != "") {
      targetColor[i] = val.toInt();
    }
  }
  adjustColor();
  webServer.send(200, "text/html", webpage);
}

void handleStatus() {
  Serial.println("Handle Status..");
  String result = ('[' + String(currentColor[0]) + ':' + String(currentColor[1]) +':'+ String(currentColor[2]) + ']');
  webServer.send(200, "text/html", result);
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

void Blink(){
  analogWrite(REDPIN, 1023); // R on
  analogWrite(GREENPIN, 1023); // G on
  analogWrite(BLUEPIN, 1023); // B on
  delay(500);
  analogWrite(REDPIN, 0); // R off
  analogWrite(GREENPIN, 0); // G off
  analogWrite(BLUEPIN, 0); // B off
  delay(500);
  analogWrite(REDPIN, 1023); // R on
  analogWrite(GREENPIN, 1023); // G on
  analogWrite(BLUEPIN, 1023); // B on
  delay(500);
  analogWrite(REDPIN, 0); // R off
  analogWrite(GREENPIN, 0); // G off
  analogWrite(BLUEPIN, 0); // B off
  delay(500);
  analogWrite(REDPIN, 1023); // R on
  analogWrite(GREENPIN, 1023); // G on
  analogWrite(BLUEPIN, 1023); // B on
  delay(500);
  analogWrite(REDPIN, 0); // R off
  analogWrite(GREENPIN, 0); // G off
  analogWrite(BLUEPIN, 0); // B off
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
    //Local intialization. Once its business is done, there is no need to keep it around

    //reset settings - for testing
    wifiManager.resetSettings();

wifiManager.addParameter(&custom_host_nr);
wifiManager.addParameter(&custom_host_nr_choice);

    analogWrite(REDPIN, 1023);

    //Blink();
    
    //sets timeout until configuration portal gets turned off
    //useful to make it all retry or go to sleep
    //in seconds
    //wifiManager.setTimeout(120);

    //WITHOUT THIS THE AP DOES NOT SEEM TO WORK PROPERLY WITH SDK 1.5 , update to at least 1.5.1
    //WiFi.mode(WIFI_STA);

    //it starts an access point with the specified name
    //here  "WiFiDomo"
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
      //Blink();
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
    webServer.begin();
    testRGB();
    Mode = 1;
    MDNS.update();
  }
  if(Mode == 1){
    webServer.handleClient();
    MDNS.update();
  }
}
