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

#include "PersistentStorage.h"


// ====== Initialize Persistent Storage ======
PersistentStorage *storage = storage->getInstance();


// ====== Factory Reset Settings ======
#define FACTORY_RESET_TIME 5000
#define FACTORY_RESET_PIN 14


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

#ifndef HYSTERESIS
#define HYSTERESIS 2
#endif

#ifndef STATE_CHANGE_DELAY
#define STATE_CHANGE_DELAY 60000
#endif


// ====== Globals ======
enum ThermostatMode {
  MANUAL = 0,
  AUTOMATIC = 1
};

enum ThermostatState {
  OFF = 0,
  HEATING = 1
};

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
           "{ \"environment\": { \"temperature\": %0.2f, \"humidity\": %0.2f }, \"setpoint\": %d, \"mode\": { \"description\": \"%s\", \"value\": %d }, \"state\": { \"description\": \"%s\", \"value\": %d } }",
           currentTemperature, currentHumidity, storage->getCurrentSetpoint(), storage->getCurrentThermostatMode() == AUTOMATIC ? "automatic" : "manual", storage->getCurrentThermostatMode(), storage->getCurrentThermostatState() == OFF ? "off" : "heating", storage->getCurrentThermostatState());

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
      storage->setCurrentThermostatMode(MANUAL);

      //return response
      server.send(400, "text/plain", "Set mode to: " + mode);

    }
    else if (mode == "auto" || mode == "automatic") {
      //set mode
      storage->setCurrentThermostatMode(AUTOMATIC);

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
      storage->setCurrentThermostatMode(MANUAL);

      //update state
      storage->setCurrentThermostatState(OFF);

      //return response
      server.send(200, "text/plain", "State updated to: " + state);
    }
    else if (state == "heating") {
      storage->setCurrentThermostatMode(MANUAL);

      //update state
      storage->setCurrentThermostatState(HEATING);

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
      storage->setCurrentThermostatMode(AUTOMATIC);

      //update setpoint
      storage->setCurrentSetpoint(setpoint);

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

double celsiusToFahrenheit(double celsius) {
  return (celsius * 9 / 5) + 32;
}

void(* resetFunc) (void) = 0; //declare reset function @ address 0

void factoryReset() {
  storage->setCurrentThermostatMode((uint8_t)AUTOMATIC);
  storage->setCurrentThermostatState((int8_t)OFF);
  storage->setCurrentSetpoint(DEFAULT_SETPOINT);
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

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Factory reset
  pinMode(FACTORY_RESET_PIN, INPUT);
  if (digitalRead(FACTORY_RESET_PIN) == HIGH) {
    unsigned long factoryResetStartTime = millis();
    while (millis() < factoryResetStartTime + FACTORY_RESET_TIME) {
      // Display factory reset
      display.clearDisplay();
      display.setCursor(30, 30);
      display.print("Factory reset in: ");
      display.setCursor(60, 40);
      display.print(ceil(((factoryResetStartTime + FACTORY_RESET_TIME) - millis()) / 1000));
      display.display();
    }
    if (digitalRead(FACTORY_RESET_PIN) == HIGH) {
      display.clearDisplay();
      display.setCursor(30, 30);
      display.print("Resetting...");
      display.display();
      delay(200);
      factoryReset();
      display.clearDisplay();
      display.setCursor(30, 30);
      display.print("Reset!");
      display.display();
      while (digitalRead(FACTORY_RESET_PIN) == HIGH) {
        delay(100);
      }
      resetFunc();

    }
  }

  //show welcome screen
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
    display.print("Connecting to network: ");
    display.print(ssid);
    display.setCursor(30, 30);

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
  display.setCursor(10, 40);
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
unsigned long lastStateChangeTime = 0;

void loop() {
  // Update WiFi
  server.handleClient();
  MDNS.update();


  // Mode state machine
  if ((ThermostatMode)storage->getCurrentThermostatMode() == MANUAL) {
    //Event based, do nothing in main loop
  }
  else if ((ThermostatMode)storage->getCurrentThermostatMode() == AUTOMATIC) {
    // Limit state update rate
    if (millis() >= lastStateChangeTime + STATE_CHANGE_DELAY) {
      lastStateChangeTime = millis();

      // Update thermostat state
      if ((uint8_t)celsiusToFahrenheit(currentTemperature) >= storage->getCurrentSetpoint() + HYSTERESIS) {
        // Turn off heat
        storage->setCurrentThermostatState(OFF);
      }
      else if ((uint8_t)celsiusToFahrenheit(currentTemperature) <= storage->getCurrentSetpoint() - HYSTERESIS) {
        // Turn on heat
        storage->setCurrentThermostatState(HEATING);
      }
    }
  }


  //State Machine
  if ((ThermostatState)storage->getCurrentThermostatState() == OFF) {
    digitalWrite(FURNACE_RELAY_PIN, LOW);
  }
  else if ((ThermostatState)storage->getCurrentThermostatState() == HEATING) {
    digitalWrite(FURNACE_RELAY_PIN, HIGH);
  }


  // Update temperature and humidity
  if (millis() >= lastDHTUpdateTime + DHT_UPDATE_PERIOD) {
    lastDHTUpdateTime = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float tempHumidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float tempTemperature = dht.readTemperature();

    if (isnan(tempHumidity) || isnan(tempTemperature)) {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else {
      currentHumidity = tempHumidity;
      currentTemperature = tempTemperature;

      Serial.print("Temp: ");
      Serial.print(currentTemperature);
      Serial.print("°C    Hum: ");
      Serial.print(currentHumidity);
      Serial.println("%");
    }
  }
  

  // Clear the screen buffer
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  //current temperature
  display.setTextSize(3);
  display.setCursor(10, 12);
  display.print((uint8_t)celsiusToFahrenheit(currentTemperature));
  display.print("F");

  //humidity
  display.setTextSize(2);
  display.setCursor(80, 5);
  display.print((uint8_t)currentHumidity);
  display.print("%");

  if ((ThermostatMode)storage->getCurrentThermostatMode() == AUTOMATIC) {
    //setpoint
    display.setTextSize(2);
    display.setCursor(80, 25);
    display.print(storage->getCurrentSetpoint());
    display.print("F");
  }

  //mode
  display.setTextSize(1);
  display.setCursor(20, 50);
  display.print((ThermostatMode)storage->getCurrentThermostatMode() == MANUAL ? "Manual" : "Auto");

  //state
  display.setTextSize(1);
  display.setCursor(80, 50);
  display.print((ThermostatState)storage->getCurrentThermostatState() == OFF ? "Off" : "Heat");

  // Update screen
  display.display();
}
