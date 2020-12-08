#include "PersistentStorage.h"

// ====== Thermostat Settings ======
#ifndef MINIMUM_SETPOINT
#define MINIMUM_SETPOINT 64
#define MAXIMUM_SETPOINT 90
#endif

#ifndef DEFAULT_SETPOINT
#define DEFAULT_SETPOINT 70
#endif

#ifndef HYSTERESIS
#define HYSTERESIS 2
#endif

#ifndef STATE_CHANGE_DELAY
#define STATE_CHANGE_DELAY 30000
#endif


double celsiusToFahrenheit(double celsius) {
  return (celsius * 9 / 5) + 32;
}

double fahrenheitToCelsius(double fahrenheit) {
  return (fahrenheit - 32) * 5 / 9;
}

class Thermostat {
  private:
    PersistentStorage *storage = storage->getInstance();

    uint8_t SETPOINT_MIN;
    uint8_t SETPOINT_MAX;
    
    unsigned long lastStateChangeTime;

    static Thermostat *instance;

  public:
    enum ThermostatMode {
      MANUAL = 0,
      AUTOMATIC = 1
    };

    enum ThermostatState {
      OFF = 0,
      HEATING = 1
    };

    
    // Singleton
    static Thermostat *getInstance() {
      if (!instance) {
        instance = new Thermostat;
      }
      return instance;
    }    

    Thermostat() {
      lastStateChangeTime = 0;

      SETPOINT_MIN = MINIMUM_SETPOINT;
      SETPOINT_MAX = MAXIMUM_SETPOINT;

      // ====== Initialize setpoint limits ======
      if (SETPOINT_MIN < 64) {
        SETPOINT_MIN = 64;
      }

      if (SETPOINT_MAX < SETPOINT_MIN + 10 || SETPOINT_MAX > 90) {
        SETPOINT_MAX = 90;
      }
    }
    

    ThermostatState update(double currentTemperature) {
      // Mode state machine
      if (getMode() == MANUAL) {
        //Do nothing
      }
      else if (getMode() == AUTOMATIC) {
        // Limit state update rate
        if (millis() >= lastStateChangeTime + STATE_CHANGE_DELAY) {
          lastStateChangeTime = millis();

          // Update thermostat state
          if ((uint8_t)celsiusToFahrenheit(currentTemperature) >= getSetpoint() + HYSTERESIS) {
            // Turn off heat
            setState(OFF);
          }
          else if ((uint8_t)celsiusToFahrenheit(currentTemperature) <= getSetpoint() - HYSTERESIS) {
            // Turn on heat
            setState(HEATING);
          }
        }
      }

      return getState();
    }
    
    //Function to factory reset the EEPROM for the thermostat
    void factoryReset() {
      storage->setCurrentThermostatMode((uint8_t)AUTOMATIC);
      storage->setCurrentThermostatState((int8_t)OFF);
      storage->setCurrentSetpoint(DEFAULT_SETPOINT);
    }
    

    // ====== Setters & Getters ======
    bool setSetpoint(double setpoint) {      
      if (setpoint >= SETPOINT_MIN && setpoint <= SETPOINT_MAX) {
        storage->setCurrentSetpoint(setpoint);
        return true;
      }

      return false;      
    }

    double getSetpoint() {
      return storage->getCurrentSetpoint();
    }

    void setMode(ThermostatMode mode) {
      //set current mode
      storage->setCurrentThermostatMode((uint8_t)mode);
    }

    ThermostatMode getMode() {
      return (ThermostatMode)storage->getCurrentThermostatMode();
    }

    void setState(ThermostatState state) {
      //set mode to manual
      storage->setCurrentThermostatMode((uint8_t)MANUAL);
      //set current state
      storage->setCurrentThermostatState((int8_t)state);
    }

    ThermostatState getState() {
      return (ThermostatState)storage->getCurrentThermostatState();
    }

};
