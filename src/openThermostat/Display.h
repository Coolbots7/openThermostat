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

        //show welcome screen
        display->setCursor(35, 20);
        display->print("Welcome to");
        display->setCursor(15, 40);
        display->print("Open Thermostat");
        display->display();

        //TODO define
        delay(1500);
    }

    // Wifi connecting
    void wifiConnecting(String ssid)
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        display->clearDisplay();
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);
        display->setCursor(5, 20);
        display->print("Connecting to network: ");
        display->print(ssid);
        display->setCursor(30, 30);

        display->print(wifiLoading ? "-" : "|");
        wifiLoading = !wifiLoading;
        display->display();
    }

    //TODO Wifi connected
    void wifiConnected(String ssid, String ip)
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        // Show wifi connected and details on screen
        display->clearDisplay();
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);
        display->setCursor(40, 20);
        display->print("Connected!");
        display->setCursor(10, 40);
        display->print("IP: ");
        display->print(WiFi.localIP());
        display->display();
        delay(1000);
    }

    //TODO main
    void main(double currentTemperature, double currentHumidity)
    {
        display->setTextSize(1);
        display->setTextColor(SSD1306_WHITE);

        // Clear the screen buffer
        display->clearDisplay();

        display->setTextColor(SSD1306_WHITE);

        //current temperature
        display->setTextSize(3);
        display->setCursor(7, 12);
        if (isnan(currentTemperature))
        {
            display->print("NAN");
        }
        else
        {
            if (storage->getSettingScreenImperial())
            {
                display->print((uint8_t)round(celsiusToFahrenheit(currentTemperature)));
                display->print("F");
            }
            else
            {
                display->print((uint8_t)round(currentTemperature));
                display->print("C");
            }
        }
        //Show that temperature is remote
        if (storage->getSettingUseRemoteTemperature())
        {
            display->setTextSize(1);
            display->print("R");
        }

        //humidity
        display->setTextSize(2);
        display->setCursor(80, 5);
        if (isnan(currentHumidity))
        {
            display->print("NAN");
        }
        else
        {
            display->print((uint8_t)currentHumidity);
            display->print("%");
        }

        if ((Thermostat::ThermostatMode)thermostat->getMode() == Thermostat::ThermostatMode::AUTOMATIC)
        {
            //setpoint
            display->setTextSize(2);
            display->setCursor(80, 25);
            if (storage->getSettingScreenImperial())
            {
                display->print((uint8_t)celsiusToFahrenheit(thermostat->getSetpoint()));
                display->print("F");
            }
            else
            {
                display->print((uint8_t)thermostat->getSetpoint());
                display->print("C");
            }
        }

        //mode
        display->setTextSize(1);
        display->setCursor(20, 50);
        display->print(thermostat->getModeString());

        //state
        display->setTextSize(1);
        display->setCursor(80, 50);
        display->print(thermostat->getStateString());

        //Update screen
        display->display();
    }
};

#endif