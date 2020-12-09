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

#include "Temperature.h"
#include "PersistentStorage.h"
#include "Thermostat.h"
#include "WebService.h"

// ====== Factory Reset Settings ======
#define FACTORY_RESET_TIME 5000
#define FACTORY_RESET_PIN 14

// ====== WiFi Settings ======
// Override SSID and password definitions in wifi.h which is excluded from source control
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

#ifndef PORT
#define PORT 80
#endif

// ====== Screen Settings ======
#define SCREEN_ADDR 0x3C

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== Relay Settings ======
#define FURNACE_RELAY_PIN 16

// ====== Temperature Sensor Settings ======
#define DHT_PIN 0

#define DHT_TYPE DHT11 // DHT 11
//#define DHT_TYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHT_TYPE DHT21   // DHT 21 (AM2301)

#define DHT_UPDATE_PERIOD 2000

DHT dht(DHT_PIN, DHT_TYPE);

// ====== Globals ======

float currentTemperature = NAN;
float currentHumidity = NAN;

//TODO factory reset settings
PersistentStorage *storage;

// Thermostat *thermostat = thermostat->getInstance();
Thermostat *thermostat = new Thermostat();

WebService *webService;

void (*resetFunc)(void) = 0; //declare reset function @ address 0

void setup()
{
  Serial.begin(115200);

  // ====== Initialize relays ======
  pinMode(FURNACE_RELAY_PIN, OUTPUT);
  digitalWrite(FURNACE_RELAY_PIN, LOW);

  // ======Initialize screen ======
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR))
  {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ; // Don't proceed, loop forever
  }

  // Clear the screen buffer
  display.clearDisplay();

  // Update screen
  display.display();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Factory reset
  pinMode(FACTORY_RESET_PIN, INPUT);
  if (digitalRead(FACTORY_RESET_PIN) == HIGH)
  {
    unsigned long factoryResetStartTime = millis();
    while (millis() < factoryResetStartTime + FACTORY_RESET_TIME)
    {
      // Display factory reset
      display.clearDisplay();
      display.setCursor(30, 30);
      display.print("Factory reset in: ");
      display.setCursor(60, 40);
      display.print(ceil(((factoryResetStartTime + FACTORY_RESET_TIME) - millis()) / 1000));
      display.display();
    }
    if (digitalRead(FACTORY_RESET_PIN) == HIGH)
    {
      display.clearDisplay();
      display.setCursor(30, 30);
      display.print("Resetting...");
      display.display();
      delay(200);
      thermostat->factoryReset();
      storage->factoryReset();
      display.clearDisplay();
      display.setCursor(30, 30);
      display.print("Reset!");
      display.display();
      while (digitalRead(FACTORY_RESET_PIN) == HIGH)
      {
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
  //TODO move WiFi connection to WebServer class while still showing progress on screen
  const char *ssid = STASSID;
  const char *password = STAPSK;
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Connecting to WiFi network: ");
  Serial.print(ssid);

  bool wifiLoading = false;
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
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

  if (MDNS.begin("esp8266"))
  {
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

  // ====== Initialize Web Service ======
  int port = PORT;
  webService = new WebService(port, thermostat);

  // ====== Initialize temperature sensor ======
  // TODO show DHT initialization on screen
  dht.begin();

  // ====== Get Persistent Storage singleton
  storage = storage->getInstance();
}

unsigned long lastDHTUpdateTime = 0;

void loop()
{

  // Update temperature and humidity
  if (millis() >= lastDHTUpdateTime + DHT_UPDATE_PERIOD)
  {
    lastDHTUpdateTime = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float tempHumidity = dht.readHumidity();
    // Read temperature as Celsius (the default)
    float tempTemperature = dht.readTemperature();

    if (isnan(tempHumidity) || isnan(tempTemperature))
    {
      Serial.println(F("Failed to read from DHT sensor!"));
    }
    else
    {
      currentHumidity = tempHumidity;
      currentTemperature = tempTemperature;

      Serial.print("Temp: ");
      Serial.print(currentTemperature);
      Serial.print("°C    Hum: ");
      Serial.print(currentHumidity);
      Serial.println("%");
    }
  }

  // Check remote temperature
  if (storage->getSettingUseRemoteTemperature())
  {
    currentTemperature = webService->getRemoteTemperature();
  }

  // Update WiFi
  webService->update(currentTemperature, currentHumidity);

  //Update thermostat
  Thermostat::ThermostatState state = thermostat->update(currentTemperature);
  if (state == Thermostat::ThermostatState::OFF)
  {
    digitalWrite(FURNACE_RELAY_PIN, LOW);
  }
  else if (state == Thermostat::ThermostatState::HEATING)
  {
    digitalWrite(FURNACE_RELAY_PIN, HIGH);
  }

  // ====== Update Display ======
  // Clear the screen buffer
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);

  //current temperature
  display.setTextSize(3);
  display.setCursor(7, 12);
  if (isnan(currentTemperature))
  {
    display.print("NAN");
  }
  else
  {
    if (storage->getSettingScreenImperial())
    {
      display.print((uint8_t)round(celsiusToFahrenheit(currentTemperature)));
      display.print("F");
    }
    else
    {
      display.print((uint8_t)round(currentTemperature));
      display.print("C");
    }
  }
  //Show that temperature is remote
  if (storage->getSettingUseRemoteTemperature())
  {
    display.setTextSize(1);
    display.print("R");
  }

  //humidity
  display.setTextSize(2);
  display.setCursor(80, 5);
  if (isnan(currentHumidity))
  {
    display.print("NAN");
  }
  else
  {
    display.print((uint8_t)currentHumidity);
    display.print("%");
  }

  if ((Thermostat::ThermostatMode)thermostat->getMode() == Thermostat::ThermostatMode::AUTOMATIC)
  {
    //setpoint
    display.setTextSize(2);
    display.setCursor(80, 25);
    if (storage->getSettingScreenImperial())
    {
      display.print((uint8_t)celsiusToFahrenheit(thermostat->getSetpoint()));
      display.print("F");
    }
    else
    {
      display.print((uint8_t)thermostat->getSetpoint());
      display.print("C");
    }
  }

  //mode
  display.setTextSize(1);
  display.setCursor(20, 50);
  display.print((Thermostat::ThermostatMode)thermostat->getMode() == Thermostat::ThermostatMode::MANUAL ? "Manual" : "Auto");

  //state
  display.setTextSize(1);
  display.setCursor(80, 50);
  display.print((Thermostat::ThermostatState)thermostat->getState() == Thermostat::ThermostatState::OFF ? "Off" : "Heat");

  //Update screen
  display.display();
}
