#ifndef DISPLAY_H
#define DISPLAY_H

#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library

#include "Temperature.h"
#include "PersistentStorage.h"
#include "Thermostat.h"

// ====== Screen Hardware Settings ======
// #define SCREEN_WIDTH 320  // OLED display width, in pixels
// #define SCREEN_HEIGHT 240 // OLED display height, in pixels

#define TFT_WIDTH 320  // ST7789 240 x 240 and 240 x 320
#define TFT_HEIGHT 240 // ST7789 240 x 320

// Definitions for ST7789V display using SPI

#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

//PCB v0.1.0
// #define TFT_CS   35  // Chip select control pin
// #define TFT_DC    32  // Data Command control pin
// #define TFT_RST   34  // Reset pin (could connect to RST pin)

// PCB Headers
#define TFT_CS 13  // Chip select control pin
#define TFT_DC 17  // Data Command control pin
#define TFT_RST 12 // Reset pin (could connect to RST pin)

// #define SCREEN_CS 13
// #define SCREEN_RESET 12
// #define SCREEN_DC 17
// #define SCREEN_MOSI 23
// #define SCREEN_MISO 19
// #define SCREEN_SCLK 18

#define SCREEN_ORIENTATION 1

// ====== Backlight Settings ======
#define SCREEN_BACKLIGHT_PIN 25

// use first channel of 16 channels (started from zero)
#define LEDC_CHANNEL_0 0

// use 13 bit precission for LEDC timer
#define LEDC_TIMER_13_BIT 13

// use 5000 Hz as a LEDC base frequency
#define LEDC_BASE_FREQ 5000

// ====== Animation Settings ======
#define TFT_FONT 2

#define WELCOME_PAUSE 1500
#define WIFI_CONNECTED_PAUSE 1500

class Display
{
private:
  TFT_eSPI *tft;

  PersistentStorage *storage;

  Thermostat *thermostat;

  bool wifiLoading;

  double brightness;

public:
  Display()
  {
    // ======Initialize Screen ======
    tft = new TFT_eSPI();
    tft->init();
    tft->setRotation(SCREEN_ORIENTATION);
    tft->setTextSize(1.5);
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK); // Adding a background colour erases previous text automatically

    // ====== Initialize Backlight ======
    // Setup timer and attach timer to a led pin
    ledcSetup(LEDC_CHANNEL_0, LEDC_BASE_FREQ, LEDC_TIMER_13_BIT);
    ledcAttachPin(SCREEN_BACKLIGHT_PIN, LEDC_CHANNEL_0);

    storage = storage->getInstance();

    thermostat = thermostat->getInstance();
  }

  // Factory Reset Pending
  void factoryResetPending(String countdown)
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    String text = "Factory reset in: " + countdown + "s";
    tft->drawCentreString(text, TFT_WIDTH / 2, TFT_HEIGHT / 2, TFT_FONT);
  }

  // Factory Resetting
  void factoryResetting()
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    tft->drawCentreString("Resetting...", TFT_WIDTH / 2, TFT_HEIGHT / 2, TFT_FONT);
  }

  // Factory Reset
  void factoryResetComplete()
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    tft->drawCentreString("Factory Reset Complate!", TFT_WIDTH / 2, TFT_HEIGHT / 2, TFT_FONT);
  }

  // Welcome
  void welcome()
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    tft->drawCentreString("Welcome to openThermostat", TFT_WIDTH / 2, TFT_HEIGHT / 2, TFT_FONT);

    delay(WELCOME_PAUSE);
  }

  // Wifi connecting
  void wifiConnecting(String ssid)
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    tft->drawCentreString("Connecting to WiFi Network: " + ssid, TFT_WIDTH / 2, TFT_HEIGHT / 2, TFT_FONT);
  }

  // Wifi connected
  void wifiConnected(String ip)
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    String ipStr = "IP: " + ip;

    tft->drawCentreString("Connected! " + ipStr, TFT_WIDTH / 2, TFT_HEIGHT / 2, TFT_FONT);

    delay(WIFI_CONNECTED_PAUSE);
  }

  //Main thermostat display
  void main(double currentTemperature, double currentHumidity)
  {
    tft->fillScreen(TFT_BLACK);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);

    //current temperature
    String tempString = "";
    if (isnan(currentTemperature))
    {
      tempString += "NAN";
    }
    else
    {
      if (storage->getSettingScreenImperial())
      {
        tempString += String((uint8_t)round(celsiusToFahrenheit(currentTemperature))) + "°F";
      }
      else
      {
        tempString += String((uint8_t)round(currentTemperature)) + "°C";
      }
    }
    //Show that temperature is remote
    if (storage->getSettingUseRemoteTemperature())
    {
      tempString += "R";
    }
    tft->setTextSize(6);
    tft->drawCentreString(tempString, 150, TFT_HEIGHT / 2 - 50, TFT_FONT);

    //humidity
    String humidityString = "";
    if (isnan(currentHumidity))
    {
      humidityString += "NAN";
    }
    else
    {
      humidityString += String((uint8_t)currentHumidity) + "%";
    }
    tft->setTextSize(2);
    tft->drawCentreString(humidityString, TFT_WIDTH / 3, TFT_HEIGHT / 2 + 50, TFT_FONT);

    //setpoint
    if ((Thermostat::ThermostatMode)thermostat->getMode() == Thermostat::ThermostatMode::AUTOMATIC)
    {
      String setpointString = "";
      if (storage->getSettingScreenImperial())
      {
        setpointString += String((uint8_t)celsiusToFahrenheit(thermostat->getSetpointLow())) + "-";
        setpointString += String((uint8_t)celsiusToFahrenheit(thermostat->getSetpointHigh())) + "°F";
      }
      else
      {
        setpointString += String((uint8_t)thermostat->getSetpointLow()) + "-";
        setpointString += String((uint8_t)thermostat->getSetpointHigh()) + "°C";
      }
      tft->setTextSize(1);
      tft->drawCentreString(setpointString, TFT_WIDTH - 60, TFT_HEIGHT / 2 + 30, TFT_FONT);
    }

    //mode
    String modeStr = thermostat->getModeString();
    modeStr.toUpperCase();
    tft->setTextSize(1);
    tft->drawCentreString(modeStr, TFT_WIDTH - 60, TFT_HEIGHT / 2 - 30, TFT_FONT);

    String stateStr = thermostat->getStateString();
    stateStr.toUpperCase();
    tft->setTextSize(1);
    tft->drawCentreString(stateStr, TFT_WIDTH - 60, TFT_HEIGHT / 2, TFT_FONT);
  }

  //Backlight brightness control
  void setBrightness(double brightness)
  {
    //cap brightness from 0.0 to 1.0
    if (brightness < 0)
    {
      brightness = 0;
    }
    if (brightness > 1)
    {
      brightness = 1;
    }

    this->brightness = brightness;

    ledcWrite(LEDC_CHANNEL_0, round(8191 * brightness));
  }

  double getBrightness()
  {
    return this->brightness;
  }
};

#endif
