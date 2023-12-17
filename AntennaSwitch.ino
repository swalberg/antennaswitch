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
  int postitions[5];
  String labels[5];
} config;

const int POSITIONS[] = { 1239, 1589, 1966, 2700, 2830 };  // 5 possible positions on the switch
const String labels1[] = { "OCF", "160M Vertical", "Grounded", "Unused", "Unused" };
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
int setpoint = POSITIONS[switch1];
int stepCount = 0;

void setup() {
  Serial.begin(115200);
  // ESP8266 wierdism... there's a shadow EEPROM we manipulate
  readConfig(config);
  WiFi.begin(config.ssid, config.password);

  if (testWifi()) {
    Serial.println("Succesfully Connected!!!");
    // Print local IP address and start web server
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    setupOta();
    //   setupRoutes();
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
    /*while (!as5600.isConnected()) {
      Serial.println("Still Waiting for the AS5600. Is it plugged in?");
      delay(500);
    }*/

    switch1 = findCurrentPosition(as5600.readAngle());
    setpoint = POSITIONS[switch1];
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
    Serial.println(as5600.readAngle());

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

    if (abs(error) <= 5) {
      stepper.move(0);
      motorActive = false;
    } else {
      motorActive = true;

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

void processWeb() {
  /*  WiFiClient client = server.available();  // Listen for incoming clients
  if (client) {                            // If a new client connects,
    String currentLine = "";               // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      // if there's bytes to read from the client,
      char c = client.read();  // read a byte, then
      request += c;
      if (c == '\n') {  // if the byte is a newline character
        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (currentLine.length() == 0) {

          if (request.indexOf("GET /1/") >= 0) {
            for (int i = 0; i < sizeof(POSITIONS) / sizeof(POSITIONS[0]); i++) {
              sprintf(buffer, "GET /1/%d", i);
              if (request.indexOf(buffer) >= 0) {
                //Serial.println(buffer);
                switch1 = i;
                setpoint = POSITIONS[switch1];
                break;
              }
            }
          }

          if (request.indexOf("PUT /1/") >= 0) {
            String newSetting = request.substring(7); // end of /
            
            setpoint = newSetting.toInt();
            Serial.printf("Manually settings setpoint to %s or %d\n", newSetting, setpoint);

          }
     
          if (request.indexOf("GET /1.json") >= 0) {
            sendResponseHeader(client, "text/json");
            displayStatusJson(client);
          } else {
            sendResponseHeader(client, "text/html");
            displayWebpage(client);
          }


          // Break out of the while loop
          break;
        } else {  // if you got a newline, then clear currentLine
          currentLine = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        currentLine += c;      // add it to the end of the currentLine
      }
    }
    // Clear the request variable
    request = "";
    // Close the connection
    client.stop();
    //Serial.printf("Client disconnected at %d.\n", currentTime);
    //Serial.println("");
  } */
}

void sendResponseHeader(WiFiClient client, String mimetype) {
  // HTTP requests always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type: " + mimetype);
  client.println("Connection: close");
  client.println();
}

void displayStatusJson(WiFiClient client) {
  client.println("{");
  client.printf("active: %d, \n", switch1);
  client.println("antennas: [");
  for (int i = 0; i < sizeof(POSITIONS) / sizeof(POSITIONS[0]); i++) {
    client.print('"' + labels1[i] + '"');
    if (i < (sizeof(POSITIONS) / sizeof(POSITIONS[0]) - 1)) {
      client.print(", ");
    }
  }
  client.println("]");

  client.println("}");
}

void displayWebpage(WiFiClient client) {
  // Display the HTML web page
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the on/off buttons
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: #77878A;}</style></head>");

  // Web Page Heading
  client.println("<body><h1>Remote Antenna Switch</h1>");


  client.println("<p>Transmit Antenna</p>");
  client.println("<table>  <tr>");
  for (int i = 0; i < sizeof(POSITIONS) / sizeof(POSITIONS[0]); i++) {
    client.println("<td>" + labels1[i] + "</td>");
  }
  client.println("</tr>");

  client.println("<tr>");
  for (int i = 0; i < sizeof(POSITIONS) / sizeof(POSITIONS[0]); i++) {
    client.print("<td>");
    if (switch1 == i) {
      client.println("<p><a href=\"#\"><button class=\"button button2\">ACTIVE</button></a></p>");
    } else {
      sprintf(buffer, "<p><a href=\"/1/%d\"><button class=\"button\">USE</button></a></p>", i);

      client.println(buffer);
    }
    client.print("</td>");
  }
  client.println("</tr></table>");
  if (motorActive) {
    client.println("<h2 style=\"background-color: red\">Moving</h2>");
  }
  if (flashMessage != "") {
    client.printf("<h2 style=\"background-color: red\">%s</h2>", flashMessage);
  }
  client.printf("<p>Current angle is %d and we're off by %d</p>", as5600.readAngle(), as5600.readAngle() - POSITIONS[switch1]);
  client.println("</body></html>");

  // The HTTP response ends with another blank line
  client.println();
}

int findCurrentPosition(int angle) {
  for (int i = 0; i < sizeof(POSITIONS) / sizeof(POSITIONS[0]); i++) {
    if (abs(angle - POSITIONS[i]) < 10) {
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