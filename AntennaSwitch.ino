// Import required libraries
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <WebSerial.h>
#include "pages.h"


#include <AccelStepper.h>
#include "AS5600.h"
#include "wire.h"

const int stepsPerRevolution = 2048;  // change this to fit the number of steps per revolution

// ULN2003 Motor Driver Pins
#define IN1 12
#define IN2 14
#define IN3 2
#define IN4 0
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

AS5600 as5600;  //  use default Wire

struct Configuration {
  char hostname[25];
  char ssid[32];
  char password[64];
  int stops[5];
  char labels[5][30];
} config;

//int POSITIONS[] = { 1239, 1589, 1966, 2700, 2830 };  // 5 possible positions on the switch, will get loaded from config
char buffer[100];
// Set web server port number to 80
AsyncWebServer server(80);

// Variable to store the HTTP request
String request;
String flashMessage;
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

int switch1 = 2;
int setpoint = config.stops[switch1];
int stepCount = 0;

void setup() {
  Serial.begin(115200);
  readConfig(config);

  WiFi.begin(config.ssid, config.password);

  if (testWifi()) {
    Serial.println("Succesfully Connected!!!");
    // Print local IP address and start web server
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    setupOta();
    setupRoutes();
    WebSerial.begin(&server);
    WebSerial.msgCallback(receiveSerialMessage);
    server.begin();
    // set the speed and acceleration
    stepper.setMaxSpeed(500);
    stepper.setSpeed(500);
    stepper.setAcceleration(500);

    as5600.begin(4);                         //  set direction pin.
    as5600.setDirection(AS5600_CLOCK_WISE);  // default, just be explicit.
    Serial.print("AS5600 connect: ");
    Serial.println(as5600.readAngle());
    while (!as5600.isConnected()) {
      Serial.println("Still Waiting for the AS5600. Is it plugged in?");
      WebSerial.println("Still Waiting for the AS5600. Is it plugged in?");
      delay(100);
    }

    switch1 = findCurrentPosition(as5600.readAngle());
    setpoint = config.stops[switch1];
    Serial.printf("Starting at position %d\n", switch1);
  } else {  // not configured or can't connect
    Serial.println("Turning the HotSpot On");
    launchConfiguration();
  }
}

int oldAngle = 0;
int loops = 0;
bool motorActive = false;
void loop() {
  loops++;
  // Only do these tasks every so often otherwise it
  // slows down the stepper motor since this executes between every step
  if (loops % 10000 == 0) {
    ArduinoOTA.handle();

    if (!as5600.isConnected()) {
      // Safety stop - if we don 't know where we are then we don' t want to drive past limits
      stepper.stop();
      //Serial.println("AS5600 is not connected!");
      flashMessage = "AS5600 is not connected!";
      return;
    }
    flashMessage = "";


    int angle = as5600.readAngle();  // 1 - 4095
    int error = setpoint - angle;
    if (loops % 100000 == 0 && motorActive) {  //                                                       1/1: Currently at                        3445 deg -> 3007     (delta -438), distance to go is 2     (22.95)
      WebSerial.printf("%d/%d: Currently at %d deg -> %d (delta %d), distance to go is %d (%0.2f)\n", as5600.isConnected(), motorActive, angle, setpoint, error, stepper.distanceToGo(), stepper.speed());
    }

    if (abs(error) <= 5 && motorActive) {
      stepper.move(0);
      motorActive = false;
    } else {
      // I know steps has nothing to do with angles but it works for now
      int newSpeed = max(5, abs(error));
      stepper.move(error > 0 ? -newSpeed : newSpeed);
    }
  }
  if (motorActive) {
    stepper.run();
  }
}

// Web serial callback for input. Doesn't do anything for now
void receiveSerialMessage(uint8_t* data, size_t len) {
  WebSerial.println("Received Web Data...");
  String d = "";
  for (int i = 0; i < len; i++) {
    d += char(data[i]);
  }
  WebSerial.println(d);
}

void setupRoutes() {
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
    WebSerial.println("Root page request");
  });
  server.on("/status.json", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "application/json", status_json, processor);
  });
  server.on("/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", config_html, processor);
  });
  server.on("/config", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ant0", true)) strcpy(config.labels[0], request->getParam("ant0", true)->value().c_str());
    if (request->hasParam("ant1", true)) strcpy(config.labels[1], request->getParam("ant1", true)->value().c_str());
    if (request->hasParam("ant2", true)) strcpy(config.labels[2], request->getParam("ant2", true)->value().c_str());
    if (request->hasParam("ant3", true)) strcpy(config.labels[3], request->getParam("ant3", true)->value().c_str());
    if (request->hasParam("ant4", true)) strcpy(config.labels[4], request->getParam("ant4", true)->value().c_str());
    if (request->hasParam("stop0", true)) config.stops[0] = atoi(request->getParam("stop0", true)->value().c_str());
    if (request->hasParam("stop1", true)) config.stops[1] = atoi(request->getParam("stop1", true)->value().c_str());
    if (request->hasParam("stop2", true)) config.stops[2] = atoi(request->getParam("stop2", true)->value().c_str());
    if (request->hasParam("stop3", true)) config.stops[3] = atoi(request->getParam("stop3", true)->value().c_str());
    if (request->hasParam("stop4", true)) config.stops[4] = atoi(request->getParam("stop4", true)->value().c_str());

    writeConfig(config);
    request->redirect("/config");
  });

  server.on("/move", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ant")) {
      moveTo(atoi(request->getParam("ant")->value().c_str()));
      WebSerial.println("Moving to " + request->getParam("ant")->value());
      request->send(200, "text/html", "{\"status\": \"ok\"}");
    } else {
      WebSerial.println("I got asked to move but no antenna given");
      request->send(412, "text/html", "{\"status\": \"need an antenna\"}");
    }
  });
}

void moveTo(int position) {
  switch1 = position;
  setpoint = config.stops[position];
  motorActive = true;
}

// Replaces placeholders in a page with dynamic info
String processor(const String& var) {

  if (var == "ANTENNA0") { return String(config.labels[0]); }
  if (var == "ANTENNA1") { return String(config.labels[1]); }
  if (var == "ANTENNA2") { return String(config.labels[2]); }
  if (var == "ANTENNA3") { return String(config.labels[3]); }
  if (var == "ANTENNA4") { return String(config.labels[4]); }
  if (var == "STOP0") { return String(config.stops[0]); }
  if (var == "STOP1") { return String(config.stops[1]); }
  if (var == "STOP2") { return String(config.stops[2]); }
  if (var == "STOP3") { return String(config.stops[3]); }
  if (var == "STOP4") { return String(config.stops[4]); }
  if (var == "ACTIVE_NUMBER") { return String(switch1); }
  if (var == "SWITCH_NAME") { return String("transmit"); }
  if (var == "FLASH_MESSAGE") { return String(flashMessage); }
  if (var == "MOTOR_STATUS") { return motorActive ? "true" : "false"; }


  if (var == "DEBUG") {
    return String("Current angle is " + String(as5600.readAngle()) + " and we're off by " + String(as5600.readAngle() - config.stops[switch1]));
  }
  return String();  // default
}


int findCurrentPosition(int angle) {
  for (int i = 0; i < sizeof(config.stops) / sizeof(config.stops[0]); i++) {
    if (abs(angle - config.stops[i]) < 10) {
      return i;
    }
  }
  return 2;  // assume middle
}

void setupOta() {
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(getHostname());

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

char* getHostname() {
  if (config.hostname == "") {
    strcpy(config.hostname, "antennarotator");
  }
  return config.hostname;
}

void readConfig(Configuration& config) {
  EEPROM.begin(512);
  EEPROM.get(0, config);
}

void writeConfig(Configuration& config) {
  EEPROM.put(0, config);
  EEPROM.commit();
}