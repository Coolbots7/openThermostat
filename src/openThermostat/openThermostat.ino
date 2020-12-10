#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "wifi.h"

#include <DHT.h>

#include "Display.h"
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
  pinMode(FURNACE_RELAY_PIN, OUTPUT);
  digitalWrite(FURNACE_RELAY_PIN, LOW);

  // ====== Create Display ======
  display = new Display();

  // Factory reset
  pinMode(FACTORY_RESET_PIN, INPUT);
  if (digitalRead(FACTORY_RESET_PIN) == HIGH)
  {
    unsigned long factoryResetStartTime = millis();
    while (millis() < factoryResetStartTime + FACTORY_RESET_TIME)
    {
      // Show factory reset pending on screen
      display->factoryResetPending(String(ceil(((factoryResetStartTime + FACTORY_RESET_TIME) - millis()) / 1000)));
    }
    if (digitalRead(FACTORY_RESET_PIN) == HIGH)
    {
      //Factory resetting on screen
      display->factoryResetting();

      thermostat->factoryReset();
      storage->factoryReset();
      delay(200);

      //Show factory reset on screen
      display->factoryResetComplete();

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

  if (MDNS.begin("esp8266"))
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
  // TODO show DHT initialization on screen
  dht.begin();

  // ====== Get Persistent Storage singleton
  storage = storage->getInstance();

  // ====== Get Thermostat singleton
  thermostat = thermostat->getInstance();

  // ====== Initialize Web Service ======
  int port = PORT;
  webService = new WebService(port);
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
  if (state == Thermostat::ThermostatState::IDLE)
  {
    digitalWrite(FURNACE_RELAY_PIN, LOW);
  }
  else if (state == Thermostat::ThermostatState::HEATING)
  {
    digitalWrite(FURNACE_RELAY_PIN, HIGH);
  }

  // Update display
  display->main(currentTemperature, currentHumidity);
}
