#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "wifi.h"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DHT.h>


// ====== WiFi Settings ======
// Override SSID and password definitions in wifi.h which is excluded from source control
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK  "your-password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);


// ====== Screen Settings ======
#define SCREEN_ADDR 0x3C

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


// ====== Relay Settings ======
#define FURNACE_RELAY_PIN 16


// ====== Temperature Sensor Settings ======
#define DHT_PIN 0

#define DHT_TYPE DHT11   // DHT 11
//#define DHT_TYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHT_TYPE DHT21   // DHT 21 (AM2301)

#define DHT_UPDATE_PERIOD 2000

DHT dht(DHT_PIN, DHT_TYPE);


// ====== Thermostat Settings ======
#ifndef MINIMUM_SETPOINT
#define MINIMUM_SETPOINT 64
#define MAXIMUM_SETPOINT 90
#endif

uint8_t SETPOINT_MIN = MINIMUM_SETPOINT;
uint8_t SETPOINT_MAX = MAXIMUM_SETPOINT;

#ifndef DEFAULT_SETPOINT
#define DEFAULT_SETPOINT 70
#endif

uint8_t currentSetpoint = DEFAULT_SETPOINT;


// ====== Globals ======
enum ThermostatMode {
  MANUAL,
  AUTOMATIC
};

enum ThermostatState {
  OFF,
  HEATING
};

// Start in AUTOMATIC
ThermostatMode currentThermostatMode = AUTOMATIC;
ThermostatState currentThermostatState = OFF;

float currentTemperature = sqrt(-1);
float currentHumidity = sqrt(-1);


String getArgValue(String argName, bool ignoreCase = false) {
  for (uint8_t i = 0; i < server.args(); i++) {
    String name = server.argName(i);

    if (ignoreCase) {
      name.toLowerCase();
      argName.toLowerCase();
    }

    if (name == argName) {
      return server.arg(i);
    }
  }

  return "";
}

void handleRoot() {
  char temp[400];
  snprintf(temp, 400,
           "{ \"environment\": { \"temperature\": %0.2f, \"humidity\": %0.2f }, \"mode\": \"%s\", \"state\": \"%s\" }",
           currentTemperature, currentHumidity, currentThermostatMode == AUTOMATIC ? "automatic" : "manual", currentThermostatState == OFF ? "off" : "heating");

  server.send(200, "application/json", temp);
}

void handleMode() {
  if (server.method() == HTTP_POST || server.method() == HTTP_PUT) {

    //Check arguments
    String mode = getArgValue("mode", true);
    if (mode.length() <= 0) {
      handleBadRequest();
      return;
    }

    mode.toLowerCase();

    if (mode == "manual") {
      //set mode
      currentThermostatMode = MANUAL;

      //return response
      server.send(400, "text/plain", "Set mode to: " + mode);

    }
    else if (mode == "auto" || mode == "automatic") {
      //set mode
      currentThermostatMode = AUTOMATIC;

      //return response
      server.send(400, "text/plain", "Set mode to: " + mode);
    }
    else {
      //return response
      server.send(400, "text/plain", "Unknown mode: " + mode);
    }

    return;
  }

  handleMethodNotAllowed();
}

void handleState() {
  if (server.method() == HTTP_POST || server.method() == HTTP_PUT) {

    //Check arguments
    String state = getArgValue("state", true);
    if (state.length() <= 0) {
      handleBadRequest();
      return;
    }

    state.toLowerCase();

    if (state == "off") {
      currentThermostatMode = MANUAL;

      //update state
      currentThermostatState = OFF;

      //return response
      server.send(200, "text/plain", "State updated to: " + state);
    }
    else if (state == "heat") {
      currentThermostatMode = MANUAL;

      //update state
      currentThermostatState = HEATING;

      //return response
      server.send(200, "text/plain", "State updated to: " + state);
    }
    else {
      //return response
      server.send(400, "text/plain", "Unknown state: " + state);
    }

    return;
  }

  handleMethodNotAllowed();
}

void handleSetpoint() {
  if (server.method() == HTTP_POST || server.method() == HTTP_PUT) {

    //Check arguments
    String setpointStr = getArgValue("setpoint", true);
    if (setpointStr.length() <= 0) {
      handleBadRequest();
      return;
    }

    // Convert argument to integer
    // Note: cast to int returns 0 if invalid
    uint8_t setpoint = setpointStr.toInt();

    if (setpoint >= SETPOINT_MIN && setpoint <= SETPOINT_MAX) {
      currentThermostatMode = AUTOMATIC;

      //update setpoint
      currentSetpoint = setpoint;

      //return response
      server.send(200, "text/plain", "Setpoint set to: " + String(setpoint));
    }
    else {
      //return response
      handleBadRequest();
    }

    return;
  }

  handleMethodNotAllowed();
}

void handleNotFound() {
  String message = "Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += methodToString(server.method());
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void handleMethodNotAllowed() {
  String message = "Method Not Allowed\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += methodToString(server.method());
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(405, "text/plain", message);
}

void handleBadRequest() {
  String message = "Bad Request\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += methodToString(server.method());
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(400, "text/plain", message);
}

String methodToString(int method) {
  switch (method) {
    case HTTP_GET:
      return "GET";
    case HTTP_POST:
      return "POST";
    case HTTP_PUT:
      return "PUT";
  }
}

void setup() {
  Serial.begin(115200);

  // ====== Initialize relays ======
  pinMode(FURNACE_RELAY_PIN, OUTPUT);
  digitalWrite(FURNACE_RELAY_PIN, LOW);


  // ======Initialize screen ======
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the screen buffer
  display.clearDisplay();

  // Update screen
  display.display();

  //show welcome screen
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(35, 20);
  display.print("Welcome to");
  display.setCursor(15, 40);
  display.print("Open Thermostat");
  display.display();

  delay(1500);

  // Clear the screen buffer
  display.clearDisplay();

  // Update screen
  display.display();


  // ====== Initialize WiFi ======
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting to WiFi network: ");
  Serial.print(ssid);

  bool wifiLoading = false;
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");

    // Show wifi status on screen
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(5, 20);
    display.print("Connecting to WiFi network: ");
    display.print(ssid);
    display.setCursor(30, 40);

    display.print(wifiLoading ? "-" : "|");
    display.display();
    wifiLoading = !wifiLoading;

    delay(500);
  }

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Show wifi connected and details on screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(40, 20);
  display.print("Connected!");
  display.setCursor(30, 40);
  display.print("IP: ");
  display.print(WiFi.localIP());
  display.display();
  delay(1000);

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/mode", handleMode);
  server.on("/setpoint", handleSetpoint);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");


  // ====== Initialize temperature sensor ======
  // TODO show DHT initialization on screen
  dht.begin();


  // ====== Initialize setpoint limits ======
  if (SETPOINT_MIN < 64) {
    SETPOINT_MIN = 64;
  }

  if (SETPOINT_MAX < SETPOINT_MIN + 10 || SETPOINT_MAX > 90) {
    SETPOINT_MAX = 90;
  }
}

unsigned long lastDHTUpdateTime = 0;

void loop() {
  // Update WiFi
  server.handleClient();
  MDNS.update();


  // Clear the screen buffer
  display.clearDisplay();

  // Set display text format
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);


  // Mode state machine
  //display current mode on screen
  display.setCursor(0, 0);
  display.print("Mode: ");
  display.print(currentThermostatMode == MANUAL ? "Manual" : "Auto");
  if (currentThermostatMode == MANUAL) {
  }
  else if (currentThermostatMode == AUTOMATIC) {
    //TODO update thermostat state
    //TODO limit state update rate
  }


  //State Machine
  display.setCursor(0, 10);
  display.print("State: ");
  display.print(currentThermostatState == OFF ? "Off" : "Heat");
  if (currentThermostatState == OFF) {
    digitalWrite(FURNACE_RELAY_PIN, LOW);
  }
  else if (currentThermostatState == HEATING) {
    digitalWrite(FURNACE_RELAY_PIN, HIGH);
  }


  // Update temperature and humidity
  if (millis() >= lastDHTUpdateTime + DHT_UPDATE_PERIOD) {
    lastDHTUpdateTime = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    currentHumidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    currentTemperature = dht.readTemperature();

    if (isnan(currentHumidity) || isnan(currentTemperature)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else {
      Serial.print("Temp: ");
      Serial.print(currentTemperature);
      Serial.print("°C    Hum: ");
      Serial.print(currentHumidity);
      Serial.println("%");
    }
  }


  if (isnan(currentHumidity) || isnan(currentTemperature)) {
    display.setCursor(0, 20);
    display.print(F("Failed to read from DHT sensor!"));
  }
  else {
    display.setCursor(0, 20);
    display.print(F("Temp: "));
    display.print(currentTemperature);
    display.print("C");

    display.setCursor(0, 30);
    display.print(F("Humidity: "));
    display.print(currentHumidity);
    display.print("%");
  }


  display.setCursor(0, 40);
  display.print("Setpoint: ");
  display.print(currentSetpoint);
  display.print("F");


  // Update screen
  display.display();
}
