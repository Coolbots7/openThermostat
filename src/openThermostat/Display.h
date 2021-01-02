#ifndef DISPLAY_H
#define DISPLAY_H

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "Temperature.h"
#include "PersistentStorage.h"
#include "Thermostat.h"

// ====== Screen Settings ======
#define SCREEN_ADDR 0x3C

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define WELCOME_PAUSE 1500
#define WIFI_CONNECTED_PAUSE 1500

class Display
{
private:
    Adafruit_SSD1306 *display;

    PersistentStorage *storage;

    Thermostat *thermostat;

    bool wifiLoading;

public:
    Display()
    {
        display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

        // ======Initialize screen ======
        if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR))
        {
            Serial.println(F("SSD1306 allocation failed"));
            for (;;)
                ; // Don't proceed, loop forever
        }
        // Clear the screen buffer
        display->clearDisplay();
        // Update screen
        display->display();

        storage = storage->getInstance();

        thermostat = thermostat->getInstance();

        wifiLoading = false;
    }

    // Factory Reset Pending
    void factoryResetPending(String countdown)
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        // Display factory reset
        display->clearDisplay();
        display->setCursor(30, 30);
        display->print("Factory reset in: ");
        display->setCursor(60, 40);
        display->print(countdown);
        display->display();
    }

    // Factory Resetting
    void factoryResetting()
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        display->clearDisplay();
        display->setCursor(30, 30);
        display->print("Resetting...");
        display->display();
    }

    // Factory Reset
    void factoryResetComplete()
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        display->clearDisplay();
        display->setCursor(30, 30);
        display->print("Reset!");
        display->display();
    }

    // Welcome
    void welcome()
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        display->clearDisplay();

        //show welcome screen
        display->setCursor(SCREEN_WIDTH / 2 - 23, SCREEN_HEIGHT / 2 - 20);
        display->print("Welcome");

        display->setCursor(SCREEN_WIDTH / 2 - 5, SCREEN_HEIGHT / 2 - 5);
        display->print("To");

        display->setCursor(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 + 10);
        display->print("openThermostat");

        display->display();

        delay(WELCOME_PAUSE);
    }

    // Wifi connecting
    void wifiConnecting(String ssid)
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        display->clearDisplay();

        display->setCursor(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 - 30);
        display->print("Connecting to");

        display->setCursor(SCREEN_WIDTH / 2 - 37, SCREEN_HEIGHT / 2 - 15);
        display->print("WiFi Network");

        display->setCursor(SCREEN_WIDTH / 2 - (ssid.length() / 2 * 8), SCREEN_HEIGHT / 2);
        display->print(ssid);

        display->setCursor(SCREEN_WIDTH / 2 - 5, SCREEN_HEIGHT / 2 + 25);
        display->print(wifiLoading ? "-" : "|");
        wifiLoading = !wifiLoading;

        display->display();
    }

    // Wifi connected
    void wifiConnected(String ip)
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        display->clearDisplay();

        display->setCursor(SCREEN_WIDTH / 2 - 29, SCREEN_HEIGHT / 2 - 15);
        display->print("Connected!");

        String ipStr = "IP: " + ip;
        display->setCursor(SCREEN_WIDTH / 2 - (ipStr.length() / 2 * 6), SCREEN_HEIGHT / 2 + 15);
        display->print(ipStr);

        display->display();

        delay(WIFI_CONNECTED_PAUSE);
    }

    //TODO main
    void main(double currentTemperature, double currentHumidity)
    {
        display->setTextColor(SSD1306_WHITE);

        // Clear the screen buffer
        display->clearDisplay();

        //current temperature
        display->setCursor(10, 4);
        if (isnan(currentTemperature))
        {
            display->setTextSize(3);
            display->print("NAN");
        }
        else
        {
            if (storage->getSettingScreenImperial())
            {
                display->setTextSize(4);
                display->print((uint8_t)round(celsiusToFahrenheit(currentTemperature)));
                display->setTextSize(3);
                display->setCursor(10 + 4 * 6 * 2, 11);
                display->print("F");
            }
            else
            {
                display->setTextSize(4);
                display->print((uint8_t)round(currentTemperature));
                display->setTextSize(3);
                display->setCursor(10 + 4 * 6 * 2, 11);
                display->print("C");
            }
        }
        //Show that temperature is remote
        if (storage->getSettingUseRemoteTemperature())
        {
            display->setCursor(10 + 4 * 6 * 2 + 6 * 3, 25);
            display->setTextSize(1);
            display->print("R");
        }

        //setpoint
        if ((Thermostat::ThermostatMode)thermostat->getMode() == Thermostat::ThermostatMode::AUTOMATIC)
        {
            if (storage->getSettingScreenImperial())
            {
                display->setCursor(90, 19);
                display->setTextSize(2);
                display->print((uint8_t)celsiusToFahrenheit(thermostat->getSetpointLow()));
                display->setTextSize(1);
                display->setCursor(90 + 6 * 2 * 2, 26);
                display->print("F");
                
                display->setCursor(90, 30);
                display->setTextSize(2);
                display->print((uint8_t)celsiusToFahrenheit(thermostat->getSetpointHigh()));
                display->setTextSize(1);
                display->setCursor(90 + 6 * 2 * 2, 37);
                display->print("F");
            }
            else
            {
                display->setCursor(90, 19);
                display->setTextSize(2);
                display->print((uint8_t)thermostat->getSetpointLow());
                display->setTextSize(1);
                display->setCursor(90 + 6 * 2 * 2, 26);
                display->print("C");

                display->setCursor(90, 30);
                display->setTextSize(2);
                display->print((uint8_t)thermostat->getSetpointHigh());
                display->setTextSize(1);
                display->setCursor(90 + 6 * 2 * 2, 37);
                display->print("C");
            }
        }

        //humidity
        display->setTextSize(2);
        display->setCursor(22, 48);
        if (isnan(currentHumidity))
        {
            display->print("NAN");
        }
        else
        {
            display->print((uint8_t)currentHumidity);
            display->setTextSize(1);
            display->setCursor(22 + 2 * 6 * 2, 55);
            display->print("%");
        }

        //mode
        String modeStr = thermostat->getModeString();
        modeStr.toUpperCase();
        display->setTextSize(1);
        display->setCursor(77, 52);
        display->print(modeStr);

        // //state
        // display->setTextSize(1);
        // display->setCursor(80, 50);
        // display->print(thermostat->getStateString());

        //Update screen
        display->display();
    }
};

#endif