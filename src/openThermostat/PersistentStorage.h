#ifndef PERSISTENT_STORAGE_H
#define PERSISTENT_STORAGE_H

#include "EEPROM.h"


// ====== Define EEPROM Size ======
#define EEPROM_SIZE 512

// ====== EEPROM Addresses ======
#define EEPROM_CURRENT_MODE 0     // 1 byte
#define EEPROM_CURRENT_STATE 1    // 1 byte
#define EEPROM_CURRENT_SETPOINT 2 // 4 bytes

#define EEPROM_SETTING_SCREEN_UNIT 40        // 1 byte
#define EEPROM_SETTING_REMOTE_TEMPERATURE 41 // 4 bytes

class PersistentStorage
{
  private:

    static PersistentStorage *instance;

    PersistentStorage()
    {
      // ====== Initialize EEPROM ======
      if (!EEPROM.begin(EEPROM_SIZE))
      {
        Serial.println("Failed to initialise EEPROM");
        for (;;) {}
      }
    }

    void EEPROM_writeDouble(uint8_t address, double value)
    {
      byte *v = (byte *)(void *)&value;
      for (int i = 0; i < sizeof(value); i++)
      {
        EEPROM.write(address + i, *v++);
      }
    }

    double EEPROM_readDouble(uint8_t address)
    {
      double value;
      byte *v = (byte *)(void *)&value;
      for (int i = 0; i < sizeof(value); i++)
      {
        *v++ = EEPROM.read(address + i);
      }
      return value;
    }

  public:
    // Singleton
    static PersistentStorage *getInstance()
    {
      if (!instance)
      {
        instance = new PersistentStorage;
      }
      return instance;
    }

    // Current Thermostat Mode
    void setCurrentThermostatMode(uint8_t mode)
    {
      EEPROM.write(EEPROM_CURRENT_MODE, mode);
      EEPROM.commit();
    }

    uint8_t getCurrentThermostatMode()
    {
      return EEPROM.read(EEPROM_CURRENT_MODE);
    }

    // Current Thermostat State
    void setCurrentThermostatState(uint8_t state)
    {
      EEPROM.write(EEPROM_CURRENT_STATE, state);
      EEPROM.commit();
    }

    uint8_t getCurrentThermostatState()
    {
      return EEPROM.read(EEPROM_CURRENT_STATE);
    }

    // Current Setpoint
    void setCurrentSetpoint(double setpoint)
    {
      EEPROM_writeDouble(EEPROM_CURRENT_SETPOINT, setpoint);
      EEPROM.commit();
    }

    double getCurrentSetpoint()
    {
      return EEPROM_readDouble(EEPROM_CURRENT_SETPOINT);
    }

    // Setting Screen Unit
    void setSettingScreenImperial(bool imperial)
    {
      EEPROM.write(EEPROM_SETTING_SCREEN_UNIT, imperial);
      EEPROM.commit();
    }

    bool getSettingScreenImperial()
    {
      return (bool)EEPROM.read(EEPROM_SETTING_SCREEN_UNIT);
    }

    // Setting use remote temperature
    void setSettingUseRemoteTemperature(bool remote)
    {
      EEPROM.write(EEPROM_SETTING_REMOTE_TEMPERATURE, remote);
      EEPROM.commit();
    }

    bool getSettingUseRemoteTemperature()
    {
      return (bool)EEPROM.read(EEPROM_SETTING_REMOTE_TEMPERATURE);
    }
};

PersistentStorage *PersistentStorage::instance = 0;

#endif
