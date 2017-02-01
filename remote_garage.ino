#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <Ticker.h> //for LED status

Ticker ticker;
#define TRIGGER_PIN 16

int door1_trigger_pin = 14; // GPIO14
int door2_trigger_pin = 12; // GPIO12

int door1_sensor_pin = 4;
int door2_sensor_pin = 5;

int door1_state = 0;
int door2_state = 0;


long buttonTimer = 0;
long longPressTime = 5000;

boolean buttonActive = false;
boolean longPressActive = false;


WiFiServer server(80);


void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

void reset_wifi_config()
{
      WiFiManager wifiManager;
      wifiManager.resetSettings();
      Serial.println("Resetting!");
      ESP.restart();
}

void on_demand_wifi()
{
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

void check_for_button()
{
  if (digitalRead(TRIGGER_PIN) == HIGH) {
    if (buttonActive == false) {
      buttonActive = true;
      buttonTimer = millis();
    }
    if ((millis() - buttonTimer > longPressTime) && (longPressActive == false)) {
      //Code for the long press
      longPressActive = true;
      reset_wifi_config();
    }
  } else {
    if (buttonActive == true) {
      if (longPressActive == true) {
        longPressActive = false;
      } else {
        //Code for the short press
      on_demand_wifi();
      }
      buttonActive = false;
    }
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
 
  pinMode(door1_trigger_pin, OUTPUT);
  pinMode(door2_trigger_pin, OUTPUT);
 
  digitalWrite(door1_trigger_pin, LOW);
  digitalWrite(door2_trigger_pin, LOW);

 


  // Start the server
  server.begin();
  Serial.println("Server started");
 
  // Print the IP address
  Serial.print("Use this URL to connect: ");
  Serial.print("http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
 
}
 
void loop() {

  check_for_button();
  //check_for_reset();




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
  if (request.indexOf("/trigger=door1") != -1)  {
    digitalWrite(door1_trigger_pin, HIGH);
    delay(300);
    digitalWrite(door1_trigger_pin, LOW);
  }
 
  if (request.indexOf("/trigger=door2") != -1)  {
    digitalWrite(door2_trigger_pin, HIGH);
    delay(300);
    digitalWrite(door2_trigger_pin, LOW);
  }

  if (request.indexOf("/checkstatus") != -1)  {
      //do nothing
  }



  // Return the response
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println(""); //  do not forget this one
  client.println("<!DOCTYPE HTML>");
  client.println("<html -webkit-text-size-adjust: 100%;>");
  
  client.println("<a href=\"/trigger=door1\"\"><button style=\"font-size:100px;height:500px;width:470px\">Trigger <br> Door 1</button></a>");
  client.println("<a href=\"/trigger=door2\"\"><button style=\"font-size:100px;height:500px;width:470px\">Trigger <br> Door 2</button></a><br /><br />");
  client.println("<a href=\"/checkstatus\"\"><button style=\"font-size:40px;height:200px;width:940px\">Check Door Status</button></a><br />");

  //client.println("<body -webkit-text-size-adjust: 100%;></body>");

  door1_state = digitalRead(door1_sensor_pin);
  door2_state = digitalRead(door2_sensor_pin);



    if (door1_state == HIGH) {
      Serial.println("Door 1 open");
      client.println("<font size=\"100\" color=red>Door1 is now: Open</font><br />");
    } else {
      Serial.println("Door 1 closed");   
      client.println("<font size=\"100\" color=green>Door1 is now: Closed</font><br />");
    }

    if (door2_state == HIGH) {
      Serial.println("Door 2 open");
      client.println("<font size=\"100\" color=red>Door2 is now: Open</font><br />");
    } else {
      Serial.println("Door 2 closed");
      client.println("<font size=\"100\" color=green>Door2 is now: Closed</font><br />");
    }
 
  
  client.println("</html>");
 
  delay(1);
  Serial.println("Client disonnected");
  Serial.println("");
 
}
 
