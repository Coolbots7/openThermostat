#include <unordered_map>
#include <string>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266Webserver.h>
#include <ESP8266mDNS.h>

#include "Temperature.h"
#include "Thermostat.h"

class WebService
{
private:
    ESP8266WebServer *server;

    PersistentStorage *storage;
    Thermostat *thermostat;

    double currentTemperature;
    double currentHumidity;

    double remoteTemperature;

    String getArgValue(String argName, bool ignoreCase = false)
    {
        for (uint8_t i = 0; i < server->args(); i++)
        {
            String name = server->argName(i);

            if (ignoreCase)
            {
                name.toLowerCase();
                argName.toLowerCase();
            }

            if (name == argName)
            {
                return server->arg(i);
            }
        }

        return "";
    }

    String statusJSON(bool useImperialUnits = false)
    {
        double temperature = currentTemperature;
        double setpoint = thermostat->getSetpoint();

        if (useImperialUnits)
        {
            temperature = celsiusToFahrenheit(temperature);
            setpoint = celsiusToFahrenheit(setpoint);
        }

        String mode = thermostat->getModeString();
        String state = thermostat->getStateString();

        char temp[400];
        snprintf(temp, 400,
                 "{ \"environment\": { \"temperature\": %0.2f, \"humidity\": %0.2f }, \"setpoint\": %0.2f, \"mode\": { \"description\": \"%s\", \"value\": %d }, \"state\": { \"description\": \"%s\", \"value\": %d } }",
                 temperature, currentHumidity, setpoint, mode.c_str(), thermostat->getMode(), state.c_str(), thermostat->getState());

        return String(temp);
    }

    String settingsJSON()
    {
        char temp[400];
        snprintf(temp, 400,
                 "{ \"screenImperial\": %s, \"useRemoteTemperature\": %s }",
                 storage->getSettingScreenImperial() ? "true" : "false", storage->getSettingUseRemoteTemperature() ? "true" : "false");

        return String(temp);
    }

    void handleRoot()
    {
        //Get url params
        String units = getArgValue("units", true);

        bool useImperialUnits = false;
        if (units.length() > 0)
        {
            units.toLowerCase();

            if (units.equals("imperial"))
            {
                useImperialUnits = true;
            }
        }

        server->send(200, "application/json", statusJSON(useImperialUnits));
    }

    void handleMode()
    {
        //Get url params
        String units = getArgValue("units", true);

        bool useImperialUnits = false;
        if (units.length() > 0)
        {
            units.toLowerCase();

            if (units.equals("imperial"))
            {
                useImperialUnits = true;
            }
        }

        if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {
            //Get url params
            String modeStr = getArgValue("mode", true);

            //Make sure required params are included
            if (modeStr.length() <= 0)
            {
                server->send(400, "application/json", statusJSON(useImperialUnits));
                return;
            }

            modeStr.toLowerCase();

            static std::unordered_map<std::string, Thermostat::ThermostatMode> const table = {
                {"off", Thermostat::ThermostatMode::OFF},
                {"heat", Thermostat::ThermostatMode::HEAT},
                {"auto", Thermostat::ThermostatMode::AUTOMATIC}};
                
            auto modeFind = table.find(modeStr.c_str());
            if (modeFind != table.end())
            {
                //Get mode from table
                Thermostat::ThermostatMode mode = modeFind->second;

                //Update mode
                thermostat->setMode(mode);

                //return response
                server->send(200, "application/json", statusJSON(useImperialUnits));
            }
            else
            {
                //return response
                server->send(400, "application/json", statusJSON(useImperialUnits));
            }

            return;
        }

        server->send(405, "application/json", statusJSON(useImperialUnits));
    }

    void handleSetpoint()
    {
        //Get url params
        String units = getArgValue("units", true);

        bool useImperialUnits = false;
        if (units.length() > 0)
        {
            units.toLowerCase();

            if (units.equals("imperial"))
            {
                useImperialUnits = true;
            }
        }

        if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {
            //Get url params
            String setpointStr = getArgValue("setpoint", true);

            //Make sure required params are included
            if (setpointStr.length() <= 0)
            {
                server->send(400, "application/json", statusJSON(useImperialUnits));
                return;
            }

            // Convert argument to integer
            // Note: cast to int returns 0 if invalid
            double setpoint = setpointStr.toDouble();

            if (useImperialUnits)
            {
                setpoint = fahrenheitToCelsius(setpoint);
            }

            //update setpoint
            if (setpoint != 0 && thermostat->setSetpoint(setpoint))
            {
                //Set thermostat to automatic
                thermostat->setMode(Thermostat::ThermostatMode::AUTOMATIC);

                //return response
                server->send(200, "application/json", statusJSON(useImperialUnits));
            }
            else
            {
                server->send(400, "application/json", statusJSON(useImperialUnits));
            }

            return;
        }

        server->send(405, "application/json", statusJSON(useImperialUnits));
    }

    void handleTemperature()
    {
        //Get url params
        String units = getArgValue("units", true);

        bool useImperialUnits = false;
        if (units.length() > 0)
        {
            units.toLowerCase();

            if (units.equals("imperial"))
            {
                useImperialUnits = true;
            }
        }

        if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {
            String temperatureStr = getArgValue("temperature", true);

            if (temperatureStr.length() <= 0)
            {
                //Bad request
                server->send(400, "application/json", statusJSON(useImperialUnits));
                return;
            }

            double temperature = temperatureStr.toDouble();
            if (useImperialUnits)
            {
                temperature = fahrenheitToCelsius(temperature);
            }

            //TODO check temperature is in a valid range

            //Update remote temperature
            remoteTemperature = temperature;
            currentTemperature = remoteTemperature;

            server->send(200, "application/json", statusJSON(useImperialUnits));
            return;
        }

        //Method not allowed
        server->send(405, "application/json", statusJSON(useImperialUnits));
    }

    void handleSettings()
    {
        if (server->method() == HTTP_GET)
        {
            server->send(200, "application/json", settingsJSON());
            return;
        }
        else if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {
            //Get screen units param and update storage
            String screenImperialStr = getArgValue("screenImperial", true);
            if (screenImperialStr.length() > 0)
            {
                screenImperialStr.toLowerCase();
                bool screenImperial = screenImperialStr.equals("true");
                storage->setSettingScreenImperial(screenImperial);
            }

            //Get use remote temperature param and update storage
            String useRemoteTemperatureStr = getArgValue("useRemoteTemperature", true);
            if (useRemoteTemperatureStr.length() > 0)
            {
                useRemoteTemperatureStr.toLowerCase();
                bool useRemoteTemperature = useRemoteTemperatureStr.equals("true");
                storage->setSettingUseRemoteTemperature(useRemoteTemperature);
            }

            server->send(200, "application/json", settingsJSON());
            return;
        }

        //Method not allowed
        server->send(405, "application/json", settingsJSON());
    }

    void handleNotFound()
    {
        server->send(404, "text/plain", "Not Found");
    }

public:
    WebService(int port)
    {
        server = new ESP8266WebServer(port);

        storage = storage->getInstance();

        thermostat = thermostat->getInstance();

        // initialize remote temperature
        remoteTemperature = NAN;

        server->on("/", std::bind(&WebService::handleRoot, this));
        server->on("/mode", std::bind(&WebService::handleMode, this));
        server->on("/setpoint", std::bind(&WebService::handleSetpoint, this));
        server->on("/temperature", std::bind(&WebService::handleTemperature, this));
        server->on("/settings", std::bind(&WebService::handleSettings, this));
        server->onNotFound(std::bind(&WebService::handleNotFound, this));
        server->begin();
    }

    void update(double temperature, double humidity)
    {
        currentTemperature = temperature;
        currentHumidity = humidity;

        server->handleClient();
        MDNS.update();
    }

    double getRemoteTemperature()
    {
        return remoteTemperature;
    }
};