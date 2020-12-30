#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "Display.h"
#include "PersistentStorage.h"
#include "Thermostat.h"
#include "WebService.h"

// End user specific config file for WiFi network settings
#include "wifi.h"

// ====== Factory Reset Settings ======
#define FACTORY_RESET_TIME 5000
#define FACTORY_RESET_PIN 2

// ====== WiFi Settings ======
// Override SSID and password definitions in wifi.h which is excluded from source control
#ifndef STASSID
#define STASSID "your-ssid"
#define STAPSK "your-password"
#endif

#ifndef PORT
#define PORT 80
#endif

// ====== Relay Settings ======
#define HEAT_RELAY_PIN 26
#define COOL_RELAY_PIN 27
#define FAN_RELAY_PIN 14


// ====== Environmental Sensor Settings ======
#define BME_CS_PIN 33
Adafruit_BME280 bme(BME_CS_PIN); // hardware SPI

#define ENVIRONMENTAL_SENSOR_UPDATE_PERIOD 2000

// ====== Globals ======

float currentTemperature = NAN;
float currentHumidity = NAN;

Display *display;

//TODO factory reset settings
PersistentStorage *storage;

Thermostat *thermostat;

WebService *webService;

void (*resetFunc)(void) = 0; //declare reset function @ address 0

void setup()
{
  Serial.begin(115200);

  // ====== Initialize relays ======
  pinMode(HEAT_RELAY_PIN, OUTPUT);
  digitalWrite(HEAT_RELAY_PIN, LOW);

  pinMode(COOL_RELAY_PIN, OUTPUT);
  digitalWrite(COOL_RELAY_PIN, LOW);

  pinMode(FAN_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, LOW);

  // ====== Create Display ======
  display = new Display();

  // Factory reset
  pinMode(FACTORY_RESET_PIN, INPUT);
  if (digitalRead(FACTORY_RESET_PIN) == HIGH)
  {
    unsigned long factoryResetStartTime = millis();
    while (millis() < factoryResetStartTime + FACTORY_RESET_TIME && digitalRead(FACTORY_RESET_PIN) == HIGH)
    {
      // Show factory reset pending on screen
      display->factoryResetPending(String(ceil(((factoryResetStartTime + FACTORY_RESET_TIME) - millis()) / 1000)));

      Serial.print("Factory resetting in ");
      Serial.print(ceil(((factoryResetStartTime + FACTORY_RESET_TIME) - millis()) / 1000));
      Serial.println(" sec");
      delay(500);
    }
    if (digitalRead(FACTORY_RESET_PIN) == HIGH)
    {
      //Factory resetting on screen
      display->factoryResetting();
      Serial.print("Resetting...");

      storage->setCurrentThermostatMode(0);
      storage->setCurrentThermostatState(0);
      storage->setCurrentSetpoint(22);
      Serial.print("thermostat reset...");

      storage->setSettingScreenImperial(false);
      storage->setSettingUseRemoteTemperature(false);
      Serial.print("storage reset...");
      delay(200);

      //Show factory reset on screen
      display->factoryResetComplete();
      Serial.println("Reset Complete!");

      while (digitalRead(FACTORY_RESET_PIN) == HIGH)
      {
        delay(100);
      }
      resetFunc();
    }
  }

  // Show welcome screen
  display->welcome();

  // ====== Initialize WiFi ======
  // TODO move WiFi connection to WebServer class while still showing progress on screen
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
    display->wifiConnecting(ssid);
    wifiLoading = !wifiLoading;

    delay(500);
  }

  if (MDNS.begin("esp32"))
  {
    Serial.println("MDNS responder started");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Show wifi connected on screen
  display->wifiConnected(WiFi.localIP().toString());

  // ====== Initialize temperature sensor ======
  // TODO show temperature initialization on screen
  if (!bme.begin())
  {
    Serial.println("Failed to initialize BME sensor");
    for (;;)
    {
    }
  }

  // ====== Get Persistent Storage singleton
  storage = storage->getInstance();

  // ====== Get Thermostat singleton
  thermostat = thermostat->getInstance();

  // ====== Initialize Web Service ======
  int port = PORT;
  webService = new WebService(port);
}

unsigned long lastEnvironemntalSensorUpdateTime = 0;

void loop()
{

  // Update temperature and humidity
  if (millis() >= lastEnvironemntalSensorUpdateTime + ENVIRONMENTAL_SENSOR_UPDATE_PERIOD)
  {
    lastEnvironemntalSensorUpdateTime = millis();

    // Reading temperature or humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
    float tempHumidity = bme.readHumidity();
    // Read temperature as Celsius (the default)
    float tempTemperature = bme.readTemperature();

    if (isnan(tempHumidity) || isnan(tempTemperature))
    {
      Serial.println(F("Failed to read from environmental sensor!"));
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
  if (state == Thermostat::ThermostatState::IDLE)
  {
    digitalWrite(HEAT_RELAY_PIN, LOW);
    digitalWrite(COOL_RELAY_PIN, LOW);
    digitalWrite(FAN_RELAY_PIN, LOW);
  }
  else if (state == Thermostat::ThermostatState::HEATING)
  {
    digitalWrite(HEAT_RELAY_PIN, HIGH);
    digitalWrite(COOL_RELAY_PIN, LOW);
    digitalWrite(FAN_RELAY_PIN, LOW);
  }
  else if(state == Thermostat::ThermostatState::COOLING) {    
    digitalWrite(HEAT_RELAY_PIN, LOW);
    digitalWrite(COOL_RELAY_PIN, HIGH);
    digitalWrite(FAN_RELAY_PIN, LOW);
  }
  else if (state == Thermostat::ThermostatState::FAN)
  {
    digitalWrite(HEAT_RELAY_PIN, LOW);
    digitalWrite(COOL_RELAY_PIN, LOW);
    digitalWrite(FAN_RELAY_PIN, HIGH);
  }
  
  // Update display
  display->main(currentTemperature, currentHumidity);
}
