#include <ESP8266WiFi.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h> //for LED status

Ticker ticker;

#define TRIGGER_PIN 0
#define WIFIRESET_PIN 2

String door1status = "open";
String door2status = "open";

int ledPin_d5 = 14; // GPIO14
int ledPin_d6 = 12; // GPIO12
int ledPin_d7 = 13; // GPIO13

int door1_sensor_pin = 4;
int door2_sensor_pin = 5;

int door1_state = 0;
int door2_state = 0;

int door1_condition = 1;
int door2_condition = 1;

WiFiServer server(80);

//LiquidCrystal_I2C lcd(0x3F, 16, 2);



void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void check_for_reset()
{
  if ( digitalRead(WIFIRESET_PIN) == LOW ) {
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      Serial.println("Resetting!");
      ESP.restart();
    }
}

void check_for_ondemand()
{
    if ( digitalRead(TRIGGER_PIN) == LOW ) {
    ticker.attach(0.2, tick);
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    if (!wifiManager.startConfigPortal("OnDemandAP")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
    }
    Serial.println("connected...yeey :)");
    ticker.detach();
  } 
}


//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

 
void setup() {
  Serial.begin(115200);
  delay(10);

  //lcd.begin(16,2);
  //lcd.init();

  // Turn on the backlight.
  //lcd.backlight();
  //lcd.setCursor(0, 0);
  //lcd.print("Open Garage");
  //lcd.setCursor(0, 1);      
  //lcd.print("Connecting Wifi");
  //delay(2000);

  pinMode(TRIGGER_PIN, INPUT);
  
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  Serial.println(WiFi.localIP());
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  pinMode(door1_sensor_pin, INPUT);
  pinMode(door2_sensor_pin, INPUT);
 
  pinMode(ledPin_d5, OUTPUT);
  pinMode(ledPin_d6, OUTPUT);
  pinMode(ledPin_d7, OUTPUT);
  digitalWrite(ledPin_d5, LOW);
  digitalWrite(ledPin_d6, LOW);
  digitalWrite(ledPin_d7, LOW);
 


  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  //lcd.clear();
  //lcd.setCursor(0, 0);
  //lcd.print("IP ADDRESS:");
  //lcd.setCursor(0, 1);      
  //lcd.print(WiFi.localIP());
  //delay(2000);

  //lcd.clear();
  //lcd.setCursor(0, 0);
  //lcd.print("Door 1: Checking");
  //lcd.setCursor(0, 1);      
  //lcd.print("Door 2: Checking");
 
}
 
void loop() {

  check_for_ondemand();
  check_for_reset();




  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

 
  // Wait until the client sends some data
  //Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
 
  // Read the first line of the request
  String request = client.readStringUntil('\r');
  //Serial.println(request);
  client.flush();



 



  // Match the request
  if (request.indexOf("/LED=d5press") != -1)  {
    digitalWrite(ledPin_d5, HIGH);
    delay(300);
    digitalWrite(ledPin_d5, LOW);
  }
 
  if (request.indexOf("/LED=d6press") != -1)  {
    digitalWrite(ledPin_d6, HIGH);
    delay(300);
    digitalWrite(ledPin_d6, LOW);
  }

  if (request.indexOf("/checkstatus") != -1)  {
      //do nothing
  }



  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  client.println("<a href=\"/LED=d5press\"\"><button>Open Door 1</button></a><br />");
  client.println("<a href=\"/LED=d6press\"\"><button>Open Door 2</button></a><br />");
  client.println("<a href=\"/checkstatus\"\"><button>Check Door Status</button></a><br />");


  door1_state = digitalRead(door1_sensor_pin);
  door2_state = digitalRead(door2_sensor_pin);




  //if (door1_condition != door1_state){
    if (door1_state == HIGH) {
      Serial.println("Door 1 open");
      //lcd.setCursor(0, 0);
      //lcd.print("Door 1: Open    ");
      client.println("Door1 is now: Open <br />");
    } else {
      Serial.println("Door 1 closed");   
      //lcd.setCursor(0, 0);
      //lcd.print("Door 1: Closed  ");
      client.println("Door1 is now: Closed <br />");
    }
    //door1_condition = door1_state;
  //}
      



  //if (door2_condition != door2_state){
    if (door2_state == HIGH) {
      Serial.println("Door 2 open");
      //lcd.setCursor(0, 1);
      //lcd.print("Door 2: Open    ");
      client.println("Door2 is now: Open <br />");
    } else {
      Serial.println("Door 2 closed");
      //lcd.setCursor(0, 1);
      //lcd.print("Door 2: Closed  ");
      client.println("Door2 is now: Closed <br />");
    }
      //door2_condition = door2_state;
 // }
 
  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}
 
