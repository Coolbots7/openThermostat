#ifndef THERMOSTAT_H
#define THERMOSTAT_H

#include "PersistentStorage.h"

// ====== Thermostat Settings ======
#ifndef MINIMUM_SETPOINT
#define MINIMUM_SETPOINT 18
#define MAXIMUM_SETPOINT 32
#endif

#define ABSOLUTE_MINIMUM_SETPOINT 18
#define ABSOLUTE_MAXIMUM_SETPOINT 32
#define MINIMUM_SETPOINT_RANGE 10

#ifndef HYSTERESIS
#define HYSTERESIS 1
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

  Thermostat()
  {
    lastStateChangeTime = 0;

    SETPOINT_MIN = MINIMUM_SETPOINT;
    SETPOINT_MAX = MAXIMUM_SETPOINT;

    // ====== Initialize setpoint limits ======
    //Cap setpoint min
    if (SETPOINT_MIN < ABSOLUTE_MINIMUM_SETPOINT)
    {
      SETPOINT_MIN = ABSOLUTE_MINIMUM_SETPOINT;
    }

    //Make sure setpoint min is no larger than ABSOLUTE_MAXIMUM_SETPOINT - MINIMUM_SETPOINT_RANGE
    //If setpoint min is greater than or equal to ABSOLUTE_MAXIMUM_SETPOINT - MINIMUM_SETPOINT_RANGE + 1 this will cause setpoint max to be less than the minimum setpoint range
    if (SETPOINT_MIN > ABSOLUTE_MAXIMUM_SETPOINT - MINIMUM_SETPOINT_RANGE)
    {
      SETPOINT_MIN = ABSOLUTE_MAXIMUM_SETPOINT - MINIMUM_SETPOINT_RANGE;
    }

    //Make sure setpoint max is at least 10 degrees higher than setpoint min
    if (SETPOINT_MAX < SETPOINT_MIN + MINIMUM_SETPOINT_RANGE)
    {
      SETPOINT_MAX = SETPOINT_MIN + MINIMUM_SETPOINT_RANGE;
    }

    //Cap setpoint max
    if (SETPOINT_MAX > ABSOLUTE_MAXIMUM_SETPOINT)
    {
      SETPOINT_MAX = ABSOLUTE_MAXIMUM_SETPOINT;
    }

    // ====== Initialize Hysteresis ======
    hysteresis = HYSTERESIS;
  }

public:
  enum ThermostatMode
  {
    OFF = 0,
    HEAT = 1,
    COOL = 2,
    AUTOMATIC = 3,
    //RESUME = 5,
    FAN_ONLY = 6
  };

  enum ThermostatState
  {
    IDLE = 0,
    HEATING = 1,
    COOLING = 2,
    FAN = 3
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

  ThermostatState update(double currentTemperature)
  {
    //check temperature is not NAN
    if (!isnan(currentTemperature))
    {

      // Mode state machine
      if (getMode() == OFF)
      {
        //set state to IDLE
        setState(IDLE);
      }
      else if (getMode() == HEAT)
      {
        //set state to HEATING
        setState(HEATING);
      }
      else if (getMode() == COOL)
      {
        //set state to COOLING
        setState(COOLING);
      }
      else if (getMode() == AUTOMATIC)
      {
        // Limit state update rate
        if (millis() >= lastStateChangeTime + STATE_CHANGE_DELAY)
        {
          lastStateChangeTime = millis();

          // Update thermostat state

          //If current temperature is greater than setpoint plus hysteresis, turn off heating          
          //If current temperature is less than setpoint minus hysteresis, turn off cooling
          if (currentTemperature >= getSetpointLow() + hysteresis && currentTemperature <= getSetpointHigh() - hysteresis)
          {
            //set state to IDLE
            setState(IDLE);
          }
          //Else, if current temperature is less than setpoint minus hysteresis, turn on heating
          else if (currentTemperature <= getSetpointLow() - hysteresis)
          {
            //set state to HEATING
            setState(HEATING);
          }
          //Else, if current  temperature is greater than setpoint plus hysteresis, turn on cooling
          else if (currentTemperature >= getSetpointHigh() + hysteresis)
          {
            //set state to COOLING
            setState(COOLING);
          }
        }
      }
      else if (getMode() == FAN_ONLY)
      {
        setState(FAN);
      }
    }

    return getState();
  }

  // ====== Setters & Getters ======
  bool setSetpointLow(double setpoint)
  {
    if (setpoint >= SETPOINT_MIN && setpoint <= SETPOINT_MAX)
    {
      storage->setSetpointLow(setpoint);
      return true;
    }

    return false;
  }

  double getSetpointLow()
  {
    return storage->getSetpointLow();
  }

  bool setSetpointHigh(double setpoint)
  {
    if (setpoint >= SETPOINT_MIN && setpoint <= SETPOINT_MAX)
    {
      storage->setSetpointHigh(setpoint);
      return true;
    }

    return false;
  }

  double getSetpointHigh()
  {
    return storage->getSetpointHigh();
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

  String getModeString()
  {
    switch ((ThermostatMode)storage->getCurrentThermostatMode())
    {
    case Thermostat::ThermostatMode::OFF:
      return "off";
    case Thermostat::ThermostatMode::HEAT:
      return "heat";
    case Thermostat::ThermostatMode::COOL:
      return "cool";
    case Thermostat::ThermostatMode::AUTOMATIC:
      return "auto";
    case Thermostat::ThermostatMode::FAN_ONLY:
      return "fan-only";
    default:
      return "";
    }
  }

  void setState(ThermostatState state)
  {
    //set current state
    storage->setCurrentThermostatState((int8_t)state);
  }

  ThermostatState getState()
  {
    return (ThermostatState)storage->getCurrentThermostatState();
  }

  String getStateString()
  {
    switch ((ThermostatState)storage->getCurrentThermostatState())
    {
    case Thermostat::ThermostatState::IDLE:
      return "idle";
    case Thermostat::ThermostatState::HEATING:
      return "heating";
    case Thermostat::ThermostatState::COOLING:
      return "cooling";
    case Thermostat::ThermostatState::FAN:
      return "fan";
    default:
      return "";
    }
  }
};

Thermostat *Thermostat::instance = 0;

#endif
