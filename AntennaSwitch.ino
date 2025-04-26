#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <TinyXML.h>
#include <WebSerial.h>
#include <WiFiUdp.h>
#include "pages.h"

const int NUM_BANDS = 11;
enum Bands {
  B_6,
  B_10,
  B_12,
  B_15,
  B_17,
  B_20,
  B_30,
  B_40,
  B_60,
  B_80,
  B_160,
  B_unknown
};
const int MAX_UDP_PACKET_SIZE = 255;
const unsigned long UNCONFIGURED_LIFE = 5 * 60 * 1000; // 5 minute timer while unconfigured
WiFiUDP UDPInfo;
unsigned int localUdpPort = 12060;
char incomingPacket[MAX_UDP_PACKET_SIZE + 1];
TinyXML xml;
uint8_t buffer[150];  // For XML decoding

#define DEFAULT_HOSTNAME "antennaswitch1"
int pins[4] = { 15, 12, 14, 13 };  // The ports on the ESP8266 don't line up to the ports on the switcher because they're out of order and I didn't notice.
int LED = 2;
struct Antenna {
  char label[30];
  byte priority;
  int bands;
};

struct Configuration {
  char hostname[25];
  char ssid[32];
  char password[64];
  Antenna antennas[4];
} config;

// Set web server port number to 80
AsyncWebServer server(80);

int loops = 0;
int activePort = 0;

// Variable to store the HTTP request
String request;
String flashMessage;

void setup() {
  Serial.begin(115200);
  readConfig(config);
  WiFi.begin(config.ssid, config.password);
  for (int i = 0; i < 4; i++) {
    pinMode(pins[i], OUTPUT);
  }
  UDPInfo.begin(localUdpPort);
  // set up xml processing
  xml.init((uint8_t*)buffer, sizeof(buffer), &XML_callback);

  if (testWifi()) {
    Serial.println("Succesfully Connected!!!");
    // Print local IP address and start web server
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.softAP("switchconfig", "");
  }
  setupOta();
  setupRoutes();
  WebSerial.begin(&server);
  WebSerial.msgCallback(receiveSerialMessage);
  server.begin();
}

void loop() {
  ArduinoOTA.handle();
  handleXMLPacket();
}

// If there's no value (default 255) in the first position, we've never been configured
bool isConfigured() {
  return EEPROM.read(0) != 255;
}

// are we connected to a wifi AP?
bool isWifiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

bool testWifi(void) {
  int c = 0;
  if (!isConfigured()) {
    return false;
  }

  Serial.println("Waiting for Wifi to connect");
  while (c < 20) {
    if (isWifiConnected()) {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

// UDP processing of port 12060
void handleXMLPacket() {
  int packetSize = UDPInfo.parsePacket();
  if (packetSize) {
    int len = UDPInfo.read(incomingPacket, MAX_UDP_PACKET_SIZE);
    xml.reset();
    for (int i = 0; i < len; i++) {
      xml.processChar(incomingPacket[i]);
    }
  }
}

void XML_callback(uint8_t statusflags, char* tagName, uint16_t tagNameLen, char* data, uint16_t dataLen) {
  if (!(statusflags & STATUS_TAG_TEXT)) {
    return;  // only care about getting the tag data
  }
  if (!strcasecmp(tagName, "/RadioInfo/Freq")) {
    WebSerial.printf("Received Packet to change to band %d which is antenna %d\n", band(atoi(data)), findBestAntenna(band(atoi(data))));
    int antenna = findBestAntenna(band(atoi(data)));
    if (antenna >= 0) {
      moveTo(antenna);
    }
  }
}

int findBestAntenna(Bands band) {
  int c = 1 << band;
  for (int i = 0; i < 4; i++) {
    //WebSerial.printf("checking antenna %d which has bands %d against %d, result is %d\n", i, config.antennas[i].bands, c, config.antennas[i].bands & c);
    if ((config.antennas[i].bands & c) > 0) {
      return i;
    }
  }
  return -1;
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

  server.on("/config_wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", config_wifi, wifi_processor);
  });
  server.on("/config_wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
      strcpy(config.hostname, request->getParam("hostname", true)->value().c_str());
      strcpy(config.ssid, request->getParam("ssid", true)->value().c_str());
      strcpy(config.password, request->getParam("pass", true)->value().c_str());
      writeConfig(config);

      request->send(200, "application/json", "{\"Success\":\"saved to eeprom... resetting to boot into new wifi\"}");
      ESP.reset();
    } else {
      request->send(409, "application/json", "{\"Error\":\"Missing a ssid and password\"}");
    }
  });

  server.on("/config", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", config_html, processor);
  });

  server.on("/config", HTTP_POST, [](AsyncWebServerRequest* request) {
    for (int i = 0; i < 4; i++) {
      String rvar = "ant";
      rvar += i;
      // Label
      if (request->hasParam(rvar, true)) {
        strcpy(config.antennas[i].label, request->getParam(rvar, true)->value().c_str());
      }
      config.antennas[i].bands = 0;  // we only get in values that are set, so start from nothing and build up
      int c = 1;
      for (int b = 0; b < NUM_BANDS; b++) {
        String band = "band_";
        band += i;
        band += "_";
        band += b;
        if (request->hasParam(band, true)) {
          if (request->getParam(band, true)->value() == "on") {
            config.antennas[i].bands |= c;  // set the bit, we support this band.
          }
        }
        c *= 2;
      }
    }

    writeConfig(config);
    request->redirect("/config");
  });

  server.on("/change", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ant")) {
      moveTo(atoi(request->getParam("ant")->value().c_str()));
      WebSerial.println("Changing to " + request->getParam("ant")->value());
      request->send(200, "text/html", "{\"status\": \"ok\"}");
    } else {
      WebSerial.println("I got asked to move but no antenna given");
      request->send(412, "text/html", "{\"status\": \"need an antenna\"}");
    }
  });
}

void moveTo(int port) {
  WebSerial.printf("> %d\n", port);
  for (int i = 0; i < 4; i++) {
    digitalWrite(pins[i], port == i ? HIGH : LOW);
  }
  activePort = port;
}

// Replaces placeholders in a page with dynamic info
String processor(const String& var) {

  if (var == "ANTENNA0") { return String(config.antennas[0].label); }
  if (var == "ANTENNA1") { return String(config.antennas[1].label); }
  if (var == "ANTENNA2") { return String(config.antennas[2].label); }
  if (var == "ANTENNA3") { return String(config.antennas[3].label); }
  if (var == "ANTENNA0BANDS") { return checkboxes(0); }
  if (var == "ANTENNA1BANDS") { return checkboxes(1); }
  if (var == "ANTENNA2BANDS") { return checkboxes(2); }
  if (var == "ANTENNA3BANDS") { return checkboxes(3); }
  if (var == "ACTIVE_NUMBER") { return String(activePort); }
  if (var == "SWITCH_NAME") { return String("transmit"); }
  if (var == "FLASH_MESSAGE") { return String(flashMessage); }
  if (var == "DEBUG") {
    String d = "";
    for (int i = 0; i < 4; i++) {
      d += config.antennas[i].bands;
      d += "|";
    }
    return d;
  }

  return String();  // default
}

// template vars for wifi setup
String wifi_processor(const String& var) {
  if (var == "FOUND_NETWORKS") {
    return String("Network scanning disabled...");
  }
  if (var == "xxxFOUND_NETWORKS") {
    int n = WiFi.scanNetworks();
    Serial.printf("There are %d networks\n", n);
    if (n == 0) {
      Serial.println("no networks found");
      return String("No networks found.");
    } else {
      String foundNetworks = String("<ol>");
      for (int i = 0; i < n; i++) {
        // Print SSID and RSSI for each network found
        foundNetworks += "<li>";
        foundNetworks += WiFi.SSID(i);
        foundNetworks += " (";
        foundNetworks += WiFi.RSSI(i);

        foundNetworks += ")";
        foundNetworks += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
        foundNetworks += "</li>";
      }
      foundNetworks += "</ol>";
      return (foundNetworks);
    }
  }

  return String();  //default
}

String checkboxes(int ant) {
  String out = "";
  int c = 1;  // compare bit
  for (int i = 0; i < NUM_BANDS; i++) {
    String value = "";

    if ((config.antennas[ant].bands & c) > 0) {
      value += "checked";
    }
    out += String("<td><input type=\"checkbox\" name =\"band_");
    out += ant;
    out += "_";
    out += i;
    out += "\" " + value + "></td>";
    c *= 2;
  }
  return out;
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

Bands band(int freq) {
  // Freq is in 10Hz, so 180740 is 1.807.40 on the dial
  freq = freq / 100;  // drop the last digit
  if (freq >= 50000) {
    return B_6;
  }
  if (freq >= 28000) {
    return B_10;
  }
  if (freq >= 24890) {
    return B_12;
  }
  if (freq >= 24890) {
    return B_12;
  }
  if (freq >= 21000) {
    return B_15;
  }
  if (freq >= 18068) {
    return B_17;
  }
  if (freq >= 14000) {
    return B_20;
  }
  if (freq >= 10100) {
    return B_30;
  }
  if (freq >= 7000) {
    return B_40;
  }
  if (freq >= 5330) {
    return B_60;
  }
  if (freq >= 3500) {
    return B_80;
  }
  if (freq >= 1800) {
    return B_160;
  }
  return B_unknown;
}