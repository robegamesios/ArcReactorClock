/*
 * theme_manager.h - Unified theme management system
 * For Multi-Mode Digital Clock project
 */

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include <TFT_eSPI.h>

// Define theme identifiers
const char* THEME_IRONMAN = "ironman";
const char* THEME_HULK = "hulk";
const char* THEME_CAPTAIN = "captain";
const char* THEME_THOR = "thor";
const char* THEME_BLACK_WIDOW = "widow";
const char* THEME_SPIDERMAN = "spiderman";

// Theme IDs for EEPROM storage
#define THEME_ID_DEFAULT 0
#define THEME_ID_IRONMAN 1
#define THEME_ID_HULK 2
#define THEME_ID_CAPTAIN 3
#define THEME_ID_THOR 4
#define THEME_ID_BLACK_WIDOW 5
#define THEME_ID_SPIDERMAN 6
#define THEME_ID_PIPBOY 7

// EEPROM storage parameters
#define EEPROM_SIZE 16
#define THEME_ADDRESS 0
#define MODE_ADDRESS 4
#define VALID_FLAG_ADDRESS 8
#define VALID_SETTINGS_FLAG 0xAA

// Clock modes
#define MODE_ARC_DIGITAL 0
#define MODE_ARC_ANALOG 1
#define MODE_PIPBOY 2
#define MODE_TOTAL 3

// LED color structure
struct LEDColors {
  int r;
  int g;
  int b;
  uint16_t tft_color;  // TFT color value for display
};

// Mode colors definitions (default values - can be changed dynamically)
LEDColors modeColors[3] = {
  { 0, 20, 255, 0x051F },  // Blue for Arc Reactor digital
  { 0, 20, 255, 0x051F },  // Blue for Arc Reactor analog (matching digital)
  { 0, 255, 50, 0x07E0 }   // Green for Pip-Boy mode
};

// Track current theme
String currentThemeName = THEME_IRONMAN;  // Default theme
int currentThemeId = THEME_ID_IRONMAN;    // Default theme ID

// External references (to be defined in main sketch)
extern int currentMode;

// Forward declarations
void initThemeSystem();
void setThemeFromFilename(const char* filename);
void setThemeFromName(const char* themeName);
void setThemeFromId(int themeId);
void saveThemeAndMode();
bool loadSavedThemeAndMode(int* loadedMode, int* loadedThemeId);
uint16_t getCurrentSecondRingColor();
String getCurrentThemeName();
int getCurrentThemeId();

// Initialize the theme system
void initThemeSystem() {
  // Initialize EEPROM with defined size
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM for theme storage!");
  } else {
    Serial.println("EEPROM initialized for theme storage");
  }
}

// Set theme based on filename (e.g., from a JPEG file)
void setThemeFromFilename(const char* filename) {
  // Create a debug message showing what filename we're checking
  Serial.print("Setting theme from filename: ");
  Serial.println(filename);

  // Create a clean version of the filename (lowercase, no path)
  String fname = String(filename);
  fname.toLowerCase();

  // Remove leading slash if present
  if (fname.startsWith("/")) {
    fname = fname.substring(1);
  }

  // First, set default theme
  currentThemeName = THEME_IRONMAN;
  currentThemeId = THEME_ID_IRONMAN;
  modeColors[0] = { 0, 20, 255, 0x051F };  // Blue digital
  modeColors[1] = { 0, 20, 255, 0x051F };  // Blue analog

  // Match theme from filename
  if (fname.indexOf(THEME_IRONMAN) >= 0) {
    // Iron Man theme - Blue for both
    modeColors[0] = { 0, 20, 255, 0x051F };  // Blue digital
    modeColors[1] = { 0, 20, 255, 0x051F };  // Blue analog
    currentThemeName = THEME_IRONMAN;
    currentThemeId = THEME_ID_IRONMAN;
    Serial.println("→ Matched Iron Man theme (blue)");
  } else if (fname.indexOf(THEME_HULK) >= 0) {
    // Hulk theme - Green for both
    modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital
    modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog
    currentThemeName = THEME_HULK;
    currentThemeId = THEME_ID_HULK;
    Serial.println("→ Matched Hulk theme (green)");
  } else if (fname.indexOf(THEME_CAPTAIN) >= 0) {
    // Captain America theme - Blue for both
    modeColors[0] = { 0, 50, 255, 0x037F };  // Blue digital
    modeColors[1] = { 0, 50, 255, 0x037F };  // Blue analog
    currentThemeName = THEME_CAPTAIN;
    currentThemeId = THEME_ID_CAPTAIN;
    Serial.println("→ Matched Captain America theme (blue)");
  } else if (fname.indexOf(THEME_THOR) >= 0) {
    // Thor theme - Yellow for both
    modeColors[0] = { 255, 255, 0, 0xFFE0 };  // Yellow digital
    modeColors[1] = { 255, 255, 0, 0xFFE0 };  // Yellow analog
    currentThemeName = THEME_THOR;
    currentThemeId = THEME_ID_THOR;
    Serial.println("→ Matched Thor theme (yellow)");
  } else if (fname.indexOf(THEME_BLACK_WIDOW) >= 0) {
    // Black Widow theme - Red for both
    modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
    modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
    currentThemeName = THEME_BLACK_WIDOW;
    currentThemeId = THEME_ID_BLACK_WIDOW;
    Serial.println("→ Matched Black Widow theme (red)");
  } else if (fname.indexOf(THEME_SPIDERMAN) >= 0) {
    // Spiderman theme - Red for both
    modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
    modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
    currentThemeName = THEME_SPIDERMAN;
    currentThemeId = THEME_ID_SPIDERMAN;
    Serial.println("→ Matched Spiderman theme (red)");
  } else {
    Serial.println("→ No specific theme matched, using default (blue)");
  }

  // Save to EEPROM if not in Pip-Boy mode
  if (currentMode != MODE_PIPBOY) {
    saveThemeAndMode();
  }
}

// Set theme based on theme name
void setThemeFromName(const char* themeName) {
  if (themeName == nullptr || strlen(themeName) == 0) {
    return;  // Don't change anything if no theme name provided
  }

  // Create a fake filename that starts with the theme name
  String fakeFilename = "/";
  fakeFilename += themeName;
  fakeFilename += "00.jpg";

  // Use the existing function to set the theme
  setThemeFromFilename(fakeFilename.c_str());
}

// Set theme based on theme ID
void setThemeFromId(int themeId) {
  const char* themeName = THEME_IRONMAN;  // Default

  switch (themeId) {
    case THEME_ID_IRONMAN:
      themeName = THEME_IRONMAN;
      break;
    case THEME_ID_HULK:
      themeName = THEME_HULK;
      break;
    case THEME_ID_CAPTAIN:
      themeName = THEME_CAPTAIN;
      break;
    case THEME_ID_THOR:
      themeName = THEME_THOR;
      break;
    case THEME_ID_BLACK_WIDOW:
      themeName = THEME_BLACK_WIDOW;
      break;
    case THEME_ID_SPIDERMAN:
      themeName = THEME_SPIDERMAN;
      break;
    case THEME_ID_PIPBOY:
      // Special case for Pip-Boy
      modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital
      modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog
      modeColors[2] = { 0, 255, 50, 0x07E0 };  // Green for Pip-Boy mode
      currentThemeName = "pipboy";
      currentThemeId = THEME_ID_PIPBOY;
      return;
  }

  setThemeFromName(themeName);
}

// Save current theme and mode to EEPROM
void saveThemeAndMode() {
  // Special handling for Pip-Boy mode
  if (currentMode == MODE_PIPBOY) {
    currentThemeId = THEME_ID_PIPBOY;
    Serial.print("Saving Pip-Boy mode with theme ID: ");
    Serial.println(currentThemeId);
  }

  Serial.print("Saving theme ID: ");
  Serial.print(currentThemeId);
  Serial.print(", Mode: ");
  Serial.println(currentMode);

  // Write values to EEPROM
  EEPROM.writeInt(THEME_ADDRESS, currentThemeId);
  EEPROM.writeInt(MODE_ADDRESS, currentMode);
  EEPROM.writeUChar(VALID_FLAG_ADDRESS, VALID_SETTINGS_FLAG);
  EEPROM.commit();
}

// Load saved theme and mode from EEPROM
// Returns true if valid settings were loaded, false otherwise
bool loadSavedThemeAndMode(int* loadedMode, int* loadedThemeId) {
  // Check if we have valid settings
  if (EEPROM.readUChar(VALID_FLAG_ADDRESS) != VALID_SETTINGS_FLAG) {
    Serial.println("No valid theme/mode settings found in EEPROM");
    return false;
  }

  // Read saved values
  int savedThemeId = EEPROM.readInt(THEME_ADDRESS);
  int savedMode = EEPROM.readInt(MODE_ADDRESS);

  // Validate the mode
  if (savedMode < 0 || savedMode >= MODE_TOTAL) {
    Serial.println("Invalid saved mode, using default");
    return false;
  }

  // Return the values through pointers
  *loadedMode = savedMode;
  *loadedThemeId = savedThemeId;

  Serial.print("Loaded saved settings - Theme ID: ");
  Serial.print(savedThemeId);
  Serial.print(", Mode: ");
  Serial.println(savedMode);

  return true;
}

// Get the current second ring color based on mode
uint16_t getCurrentSecondRingColor() {
  // Return the TFT color value for the current mode
  return modeColors[currentMode].tft_color;
}

// Return the current theme name
String getCurrentThemeName() {
  return currentThemeName;
}

// Return the current theme ID
int getCurrentThemeId() {
  return currentThemeId;
}

#endif  // THEME_MANAGER_H
