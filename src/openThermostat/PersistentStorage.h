#ifndef PERSISTENT_STORAGE_H
#define PERSISTENT_STORAGE_H

#include <EEPROM.h>

// ====== EEPROM Addresses ======
#define EEPROM_CURRENT_MODE 0     // 1 byte
#define EEPROM_CURRENT_STATE 1    // 1 byte
#define EEPROM_CURRENT_SETPOINT 2 // 4 bytes

#define EEPROM_SETTING_SCREEN_UNIT 40        // 1 byte
#define EEPROM_SETTING_REMOTE_TEMPERATURE 41 // 4 bytes

class PersistentStorage
{
private:
  uint8_t currentThermostatMode;
  uint8_t currentThermostatState;
  double currentSetpoint;

  bool settingScreenImperial;
  bool useRemoteTemperature;

  static PersistentStorage *instance;

  PersistentStorage()
  {
    // ====== Initialize EEPROM ======
    EEPROM.begin(512);

    currentThermostatMode = getCurrentThermostatMode();
    currentThermostatState = getCurrentThermostatState();
    currentSetpoint = getCurrentSetpoint();

    settingScreenImperial = getSettingScreenImperial();
    useRemoteTemperature = getSettingUseRemoteTemperature();
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

  void factoryReset()
  {
    setSettingScreenImperial(false);
  }

  // Current Thermostat Mode
  void setCurrentThermostatMode(uint8_t mode)
  {
    //Only update EEPROM if value is new to minimize writes to flash
    if (mode != currentThermostatMode)
    {
      currentThermostatMode = mode;
      EEPROM.write(EEPROM_CURRENT_MODE, currentThermostatMode);
      EEPROM.commit();
    }
  }

  uint8_t getCurrentThermostatMode()
  {
    return EEPROM.read(EEPROM_CURRENT_MODE);
  }

  // Current Thermostat State
  void setCurrentThermostatState(uint8_t state)
  {
    //Only update EEPROM if value is new to minimize writes to flash
    if (state != currentThermostatState)
    {
      currentThermostatState = state;
      EEPROM.write(EEPROM_CURRENT_STATE, currentThermostatState);
      EEPROM.commit();
    }
  }

  uint8_t getCurrentThermostatState()
  {
    return EEPROM.read(EEPROM_CURRENT_STATE);
  }

  // Current Setpoint
  void setCurrentSetpoint(double setpoint)
  {
    //Only update EEPROM if value is new to minimize writes to flash
    if (setpoint != currentSetpoint)
    {
      currentSetpoint = setpoint;
      EEPROM_writeDouble(EEPROM_CURRENT_SETPOINT, currentSetpoint);
      EEPROM.commit();
    }
  }

  double getCurrentSetpoint()
  {
    return EEPROM_readDouble(EEPROM_CURRENT_SETPOINT);
  }

  // Setting Screen Unit
  void setSettingScreenImperial(bool imperial)
  {
    if (imperial != settingScreenImperial)
    {
      settingScreenImperial = imperial;
      EEPROM.write(EEPROM_SETTING_SCREEN_UNIT, settingScreenImperial);
      EEPROM.commit();
    }
  }

  bool getSettingScreenImperial()
  {
    return (bool)EEPROM.read(EEPROM_SETTING_SCREEN_UNIT);
  }

  // Setting use remote temperature
  void setSettingUseRemoteTemperature(bool remote)
  {
    if (useRemoteTemperature != remote)
    {
      useRemoteTemperature = remote;
      EEPROM.write(EEPROM_SETTING_REMOTE_TEMPERATURE, useRemoteTemperature);
      EEPROM.commit();
    }
  }

  bool getSettingUseRemoteTemperature()
  {
    return (bool)EEPROM.read(EEPROM_SETTING_REMOTE_TEMPERATURE);
  }
};

PersistentStorage *PersistentStorage::instance = 0;

#endif