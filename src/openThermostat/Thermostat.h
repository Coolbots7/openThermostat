#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include "PersistentStorage.h"

// ====== Thermostat Settings ======
#ifndef MINIMUM_SETPOINT
#define MINIMUM_SETPOINT 18
#define MAXIMUM_SETPOINT 32
#endif

#ifndef DEFAULT_SETPOINT
#define DEFAULT_SETPOINT 22
#endif

#ifndef HYSTERESIS
#define HYSTERESIS 1.5
#endif

#ifndef STATE_CHANGE_DELAY
#define STATE_CHANGE_DELAY 30000
#endif

class Thermostat
{
private:
  PersistentStorage *storage = storage->getInstance();

  double SETPOINT_MIN;
  double SETPOINT_MAX;

  double hysteresis;

  unsigned long lastStateChangeTime;

  static Thermostat *instance;

public:
  enum ThermostatMode
  {
    MANUAL = 0,
    AUTOMATIC = 1
  };

  enum ThermostatState
  {
    OFF = 0,
    HEATING = 1
  };

  // Singleton
  static Thermostat *getInstance()
  {
    if (!instance)
    {
      instance = new Thermostat;
    }
    return instance;
  }

  Thermostat()
  {
    lastStateChangeTime = 0;

    SETPOINT_MIN = MINIMUM_SETPOINT;
    SETPOINT_MAX = MAXIMUM_SETPOINT;

    // ====== Initialize setpoint limits ======
    //Cap setpoint min
    if (SETPOINT_MIN < 18)
    {
      SETPOINT_MIN = 18;
    }

    //Make sure setpoint min is no larger than 22
    //If setpoint min is greater than or equal to 23 this will cause setpoint max to be less than 10 degrees higher
    if (SETPOINT_MIN > 22)
    {
      SETPOINT_MIN = 22;
    }

    //Make sure setpoint max is at least 10 degrees higher than setpoint min
    if (SETPOINT_MAX < SETPOINT_MIN + 10)
    {
      SETPOINT_MAX = SETPOINT_MIN + 10;
    }

    //Cap setpoint max
    if (SETPOINT_MAX > 32)
    {
      SETPOINT_MAX = 32;
    }

    // ====== Initialize Hysteresis ======
    hysteresis = HYSTERESIS;
  }

  ThermostatState update(double currentTemperature)
  {
    //check temperature is not NAN
    if (!isnan(currentTemperature))
    {

      // Mode state machine
      if (getMode() == MANUAL)
      {
        //Do nothing
      }
      else if (getMode() == AUTOMATIC)
      {
        // Limit state update rate
        if (millis() >= lastStateChangeTime + STATE_CHANGE_DELAY)
        {
          lastStateChangeTime = millis();

          // Update thermostat state
          if (currentTemperature >= getSetpoint() + hysteresis)
          {
            // Turn off heat
            setState(OFF);
          }
          else if (currentTemperature <= getSetpoint() - hysteresis)
          {
            // Turn on heat
            setState(HEATING);
          }
        }
      }
    }

    return getState();
  }

  //Function to factory reset the EEPROM for the thermostat
  void factoryReset()
  {
    storage->setCurrentThermostatMode((uint8_t)AUTOMATIC);
    storage->setCurrentThermostatState((int8_t)OFF);
    storage->setCurrentSetpoint(DEFAULT_SETPOINT);
  }

  // ====== Setters & Getters ======
  bool setSetpoint(double setpoint)
  {
    if (setpoint >= SETPOINT_MIN && setpoint <= SETPOINT_MAX)
    {
      storage->setCurrentSetpoint(setpoint);
      return true;
    }

    return false;
  }

  double getSetpoint()
  {
    return storage->getCurrentSetpoint();
  }

  void setMode(ThermostatMode mode)
  {
    //set current mode
    storage->setCurrentThermostatMode((uint8_t)mode);
  }

  ThermostatMode getMode()
  {
    return (ThermostatMode)storage->getCurrentThermostatMode();
  }

  void setState(ThermostatState state)
  {
    //set mode to manual
    storage->setCurrentThermostatMode((uint8_t)MANUAL);
    //set current state
    storage->setCurrentThermostatState((int8_t)state);
  }

  ThermostatState getState()
  {
    return (ThermostatState)storage->getCurrentThermostatState();
  }
};

#endif