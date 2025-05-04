/*
 * theme_persistence.h - Functions for saving and loading theme preferences
 * For Multi-Mode Digital Clock project
 */

#ifndef THEME_PERSISTENCE_H
#define THEME_PERSISTENCE_H

#include <EEPROM.h>
#include <Arduino.h>

// Forward declare constants to avoid circular dependencies
extern const char* THEME_IRONMAN;
extern const char* THEME_HULK;
extern const char* THEME_CAPTAIN;
extern const char* THEME_THOR;
extern const char* THEME_BLACK_WIDOW;
extern const char* THEME_SPIDERMAN;

// EEPROM storage locations
#define EEPROM_SIZE 16            // Total EEPROM size to use
#define THEME_ADDRESS 0           // Address to store theme ID
#define MODE_ADDRESS 4            // Address to store current mode
#define VALID_SETTINGS_FLAG 0xAA  // Flag to indicate valid settings

// Function to initialize EEPROM
void initThemeStorage() {
  // Initialize EEPROM with defined size
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM!");
  } else {
    Serial.println("EEPROM initialized for theme storage");
  }
}

// Save the current theme to EEPROM
void saveCurrentTheme(const char* themeName, int currentMode) {
  Serial.print("Saving theme: ");
  Serial.print(themeName);
  Serial.print(", Mode: ");
  Serial.println(currentMode);

  // Calculate a simple hash from the theme name
  uint32_t themeHash = 0;
  for (int i = 0; themeName[i] != '\0' && i < 32; i++) {
    themeHash = (themeHash << 1) ^ themeName[i];
  }

  // Write the theme hash to EEPROM
  EEPROM.writeUInt(THEME_ADDRESS, themeHash);

  // Write the current mode to EEPROM
  EEPROM.writeUInt(MODE_ADDRESS, currentMode);

  // Write a flag to indicate valid settings
  EEPROM.writeUChar(EEPROM_SIZE - 1, VALID_SETTINGS_FLAG);

  // Commit changes to flash
  EEPROM.commit();
  Serial.println("Theme saved to EEPROM");
}

// Calculate hash for a theme name (helper function)
uint32_t calculateThemeHash(const char* themeName) {
  uint32_t hash = 0;
  for (int i = 0; themeName[i] != '\0' && i < 32; i++) {
    hash = (hash << 1) ^ themeName[i];
  }
  return hash;
}

// Function to check if a saved theme matches the given name
bool isThemeHashMatch(const char* themeName) {
  if (EEPROM.readUChar(EEPROM_SIZE - 1) != VALID_SETTINGS_FLAG) {
    return false;
  }

  uint32_t savedHash = EEPROM.readUInt(THEME_ADDRESS);
  uint32_t nameHash = calculateThemeHash(themeName);

  return (savedHash == nameHash);
}

// Load the saved mode
int loadSavedMode() {
  // Check if we have valid settings
  if (EEPROM.readUChar(EEPROM_SIZE - 1) != VALID_SETTINGS_FLAG) {
    Serial.println("No valid mode settings found in EEPROM");
    return 0;  // Return default mode if no valid settings
  }

  // Read the saved mode
  int savedMode = EEPROM.readUInt(MODE_ADDRESS);

  // Validate the mode
  if (savedMode < 0) {
    Serial.println("Invalid saved mode, using default");
    return 0;
  }

  Serial.print("Loaded saved mode: ");
  Serial.println(savedMode);
  return savedMode;
}

#endif  // THEME_PERSISTENCE_H
