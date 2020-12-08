#ifndef PERSISTENT_STORAGE_H
#define PERSISTENT_STORAGE_H

#include <EEPROM.h>

// ====== EEPROM Addresses ======
#define EEPROM_CURRENT_MODE 0 // 1 byte
#define EEPROM_CURRENT_STATE 1 // 1 byte
#define EEPROM_CURRENT_SETPOINT 2 // 4 bytes

class PersistentStorage {
  private:
    uint8_t currentThermostatMode;
    uint8_t currentThermostatState;
    uint8_t currentSetpoint;

    static PersistentStorage *instance;

    PersistentStorage() {

      // ====== Initialize EEPROM ======
      EEPROM.begin(512);

      currentThermostatMode = getCurrentThermostatMode();
      currentThermostatState = getCurrentThermostatState();
      currentSetpoint = getCurrentSetpoint();
    }

  public:
    // Singleton
    static PersistentStorage *getInstance() {
      if (!instance) {
        instance = new PersistentStorage;
      }
      return instance;
    }


    // Current Thermostat Mode
    void setCurrentThermostatMode(uint8_t mode) {
      //Only update EEPROM if value is new to minimize writes to flash
      if (mode != currentThermostatMode) {
        currentThermostatMode = mode;
        EEPROM.write(EEPROM_CURRENT_MODE, currentThermostatMode);
        EEPROM.commit();
      }
    }

    uint8_t getCurrentThermostatMode() {
      return EEPROM.read(EEPROM_CURRENT_MODE);
    }


    // Current Thermostat State
    void setCurrentThermostatState(uint8_t state) {
      //Only update EEPROM if value is new to minimize writes to flash
      if (state != currentThermostatState) {
        currentThermostatState = state;
        EEPROM.write(EEPROM_CURRENT_STATE, currentThermostatState);
        EEPROM.commit();
      }
    }

    uint8_t getCurrentThermostatState() {
      return EEPROM.read(EEPROM_CURRENT_STATE);
    }


    // Current Setpoint
    void setCurrentSetpoint(uint8_t setpoint) {
      //Only update EEPROM if value is new to minimize writes to flash
      if (setpoint != currentSetpoint) {
        currentSetpoint = setpoint;
        EEPROM.write(EEPROM_CURRENT_SETPOINT, setpoint);
        EEPROM.commit();
      }
    }

    uint8_t getCurrentSetpoint() {
      return EEPROM.read(EEPROM_CURRENT_SETPOINT);
    }
};

PersistentStorage *PersistentStorage::instance = 0;

#endif