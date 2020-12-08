#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266Webserver.h>
#include <ESP8266mDNS.h>

#include "Thermostat.h"

class WebService
{
private:
    ESP8266WebServer *server;

    Thermostat *thermostat;

    double currentTemperature;
    double currentHumidity;

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

    String methodToString(int method)
    {
        switch (method)
        {
        case HTTP_GET:
            return "GET";
        case HTTP_POST:
            return "POST";
        case HTTP_PUT:
            return "PUT";
        }
    }

    String statusJSON()
    {
        char temp[400];
        snprintf(temp, 400,
                 "{ \"environment\": { \"temperature\": %0.2f, \"humidity\": %0.2f }, \"setpoint\": %0.2f, \"mode\": { \"description\": \"%s\", \"value\": %d }, \"state\": { \"description\": \"%s\", \"value\": %d } }",
                 currentTemperature, currentHumidity, fahrenheitToCelsius(thermostat->getSetpoint()), thermostat->getMode() == Thermostat::ThermostatMode::AUTOMATIC ? "automatic" : "manual", thermostat->getMode(), thermostat->getState() == Thermostat::ThermostatState::OFF ? "off" : "heating", thermostat->getState());

        return String(temp);
    }

    void handleRoot()
    {
        server->send(200, "application/json", statusJSON());
    }

    void handleMode()
    {
        if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {

            //Check arguments
            String mode = getArgValue("mode", true);
            if (mode.length() <= 0)
            {
                handleBadRequest();
                return;
            }

            mode.toLowerCase();

            if (mode == "manual")
            {
                //set mode
                thermostat->setMode(Thermostat::ThermostatMode::MANUAL);

                //return response
                server->send(200, "application/json", statusJSON());
            }
            else if (mode == "auto" || mode == "automatic")
            {
                //set mode
                thermostat->setMode(Thermostat::ThermostatMode::AUTOMATIC);

                //return response
                server->send(200, "application/json", statusJSON());
            }
            else
            {
                //return response
                server->send(400, "application/json", statusJSON());
            }

            return;
        }

        handleMethodNotAllowed();
    }

    void handleState()
    {
        if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {

            //Check arguments
            String state = getArgValue("state", true);
            if (state.length() <= 0)
            {
                handleBadRequest();
                return;
            }

            state.toLowerCase();

            if (state == "off")
            {
                //update state
                thermostat->setState(Thermostat::ThermostatState::OFF);

                //return response
                server->send(200, "application/json", statusJSON());
            }
            else if (state == "heating")
            {
                //update state
                thermostat->setState(Thermostat::ThermostatState::HEATING);

                //return response
                server->send(200, "application/json", statusJSON());
            }
            else
            {
                //return response
                server->send(400, "application/json", statusJSON());
            }

            return;
        }

        handleMethodNotAllowed();
    }

    void handleSetpoint()
    {
        if (server->method() == HTTP_POST || server->method() == HTTP_PUT)
        {

            //Check arguments
            String setpointStr = getArgValue("setpoint", true);
            if (setpointStr.length() <= 0)
            {
                handleBadRequest();
                return;
            }

            // Convert argument to integer
            // Note: cast to int returns 0 if invalid
            uint8_t setpoint = setpointStr.toInt();

            //update setpoint
            if (setpoint != 0 && thermostat->setSetpoint(setpoint))
            {
                //Set thermostat to automatic
                thermostat->setMode(Thermostat::ThermostatMode::AUTOMATIC);

                //return response
                server->send(200, "application/json", statusJSON());
            }
            else
            {
                handleBadRequest();
            }

            return;
        }

        handleMethodNotAllowed();
    }

    void handleNotFound()
    {
        String message = "Not Found\n\n";
        message += "URI: ";
        message += server->uri();
        message += "\nMethod: ";
        message += methodToString(server->method());
        message += "\nArguments: ";
        message += server->args();
        message += "\n";

        for (uint8_t i = 0; i < server->args(); i++)
        {
            message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
        }

        server->send(404, "application/json", statusJSON());
    }

    void handleMethodNotAllowed()
    {
        String message = "Method Not Allowed\n\n";
        message += "URI: ";
        message += server->uri();
        message += "\nMethod: ";
        message += methodToString(server->method());
        message += "\nArguments: ";
        message += server->args();
        message += "\n";

        for (uint8_t i = 0; i < server->args(); i++)
        {
            message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
        }

        server->send(405, "application/json", statusJSON());
    }

    void handleBadRequest()
    {
        String message = "Bad Request\n\n";
        message += "URI: ";
        message += server->uri();
        message += "\nMethod: ";
        message += methodToString(server->method());
        message += "\nArguments: ";
        message += server->args();
        message += "\n";

        for (uint8_t i = 0; i < server->args(); i++)
        {
            message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
        }

        server->send(400, "application/json", statusJSON());
    }

public:
    WebService(int port, Thermostat *t)
    {
        server = new ESP8266WebServer(port);

        thermostat = t;

        server->on("/", std::bind(&WebService::handleRoot, this));
        server->on("/state", std::bind(&WebService::handleState, this));
        server->on("/mode", std::bind(&WebService::handleMode, this));
        server->on("/setpoint", std::bind(&WebService::handleSetpoint, this));
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
};