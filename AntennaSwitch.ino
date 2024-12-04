// Import required libraries
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <WebSerial.h>
#include "pages.h"

#define DEFAULT_HOSTNAME "antennaswitch1"
int pins[4] = { 14, 12, 13, 15 };
int LED = 2;
struct Configuration {
  char hostname[25];
  char ssid[32];
  char password[64];
  char labels[4][30];
} config;

char buffer[100];
// Set web server port number to 80
AsyncWebServer server(80);

int loops = 0;
int switch1 = 0;

// Variable to store the HTTP request
String request;
String flashMessage;

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

  } else {  // not configured or can't connect
    Serial.println("Turning the HotSpot On");
    launchConfiguration();
  }
  moveTo(0); // pick first antenna so we're going out somewhere...
}

void loop() {
  loops++;
  if (loops % 10000 == 0) {
    ArduinoOTA.handle();

    flashMessage = "";
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
  allOff();
  switch1 = position;
  if (position >= 0 && position <= 3) { // ground all is a high number so will skip
    digitalWrite(pins[position], HIGH);
  }
}

// Replaces placeholders in a page with dynamic info
String processor(const String& var) {

  if (var == "ANTENNA0") { return String(config.labels[0]); }
  if (var == "ANTENNA1") { return String(config.labels[1]); }
  if (var == "ANTENNA2") { return String(config.labels[2]); }
  if (var == "ANTENNA3") { return String(config.labels[3]); }
  if (var == "ACTIVE_NUMBER") { return String(switch1); }
  if (var == "SWITCH_NAME") { return String("transmit"); }
  if (var == "FLASH_MESSAGE") { return String(flashMessage); }

  return String();  // default
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
    strcpy(config.hostname, DEFAULT_HOSTNAME);
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

void allOff() {
  for (int i = 0; i < 4; i++) {
    digitalWrite(pins[i], LOW);
  }
}