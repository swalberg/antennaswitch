String foundNetworks;

// If there's no value (default 255) in the first position, we've never been configured
bool isConfigured() {
  return EEPROM.read(0) != 255;
}

bool testWifi(void) {
  int c = 0;
  if (!isConfigured()) {
    return false;
  }

  Serial.println("Waiting for Wifi to connect");
  while (c < 20) {
    if (WiFi.status() == WL_CONNECTED) {
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

void launchConfiguration() {
  createWebServer();
  server.begin();
  setupAP();
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.printf("There are %d networks\n", n);
  if (n == 0) {
    Serial.println("no networks found");
    foundNetworks = "No networks found";
  } else {
    foundNetworks = "<ol>";
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
  }

  delay(100);
  WiFi.softAP("switchconfig", "");
}

void createWebServer() {

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    IPAddress ip = WiFi.softAPIP();
    String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);

    String content;

    content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
    content += ipStr;
    content += "!!!<p>";
    content += foundNetworks;
    content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><label>Password: </label><input name='pass' length=64><label>Hostname</label><input name='hostname'><input type='submit'></form>";
    content += "</html>";
    request->send(200, "text/html", content);
  });

  server.on("/setting", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("ssid") && request->hasParam("pass")) {
      strcpy(config.hostname, request->getParam("hostname")->value().c_str());
      strcpy(config.ssid, request->getParam("ssid")->value().c_str());
      strcpy(config.password, request->getParam("pass")->value().c_str());
      writeConfig(config);
      
      request->send(200, "application/json", "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}");
      ESP.reset();
    } else {
      request->send(409, "application/json", "{\"Error\":\"Missing a ssid and password\"}");
    }
  });
}
