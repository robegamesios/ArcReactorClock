/*
 * theme_manager.h - Unified theme management system
 * For Multi-Mode Digital Clock project
 * 
 * REVISED VERSION - Removed EEPROM dependencies and theme IDs, uses SPIFFS for storage
 */

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// Clock modes
#define MODE_ARC_DIGITAL 0
#define MODE_ARC_ANALOG 1
#define MODE_PIPBOY 2
#define MODE_TOTAL 3

// LED color IDs for consistency
#define COLOR_BLUE 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_CYAN 4
#define COLOR_PURPLE 5
#define COLOR_WHITE 6
#define COLOR_TOTAL 7

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

// Predefined LED colors array
LEDColors ledColors[COLOR_TOTAL] = {
  { 0, 20, 255, 0x051F },    // Blue
  { 255, 0, 0, 0xF800 },     // Red
  { 0, 255, 50, 0x07E0 },    // Green
  { 255, 255, 0, 0xFFE0 },   // Yellow
  { 0, 255, 255, 0x07FF },   // Cyan
  { 180, 0, 255, 0xC01F },   // Purple
  { 255, 255, 255, 0xFFFF }  // White
};

// Track current LED color
int currentLedColor = COLOR_BLUE;  // Current LED color index

// External references (to be defined in main sketch)
extern int currentMode;

// Forward declarations
void setThemeFromFilename(const char* filename);
void updateModeColorsFromLedColor(int colorIndex);
uint16_t getCurrentSecondRingColor();
void cycleLedColor();

// In theme_manager.h
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

  // Match filename to determine appropriate color
  if (fname.indexOf("ironman") >= 0) {
    currentLedColor = COLOR_BLUE;
    Serial.println("→ Set Iron Man theme (blue)");
  } else if (fname.indexOf("hulk") >= 0) {
    currentLedColor = COLOR_GREEN;
    Serial.println("→ Set Hulk theme (green)");
  } else if (fname.indexOf("captain") >= 0) {
    currentLedColor = COLOR_BLUE;
    Serial.println("→ Set Captain America theme (blue)");
  } else if (fname.indexOf("thor") >= 0) {
    currentLedColor = COLOR_YELLOW;
    Serial.println("→ Set Thor theme (yellow)");
  } else if (fname.indexOf("widow") >= 0 || fname.indexOf("spiderman") >= 0) {
    currentLedColor = COLOR_RED;
    Serial.println("→ Set red theme");
  } else {
    // Default to blue
    currentLedColor = COLOR_BLUE;
    Serial.println("→ No specific theme matched, using default (blue)");
  }

  // Update mode colors based on the selected color
  updateModeColorsFromLedColor(currentLedColor);
}

// Update mode colors based on selected color
void updateModeColorsFromLedColor(int colorIndex) {
  if (colorIndex < 0 || colorIndex >= COLOR_TOTAL) {
    colorIndex = COLOR_BLUE;  // Default to blue if invalid
  }

  currentLedColor = colorIndex;

  // Update mode colors for digital and analog modes
  if (currentMode != MODE_PIPBOY) {
    // For non-Pip-Boy modes, apply selected color
    modeColors[0] = ledColors[colorIndex];  // Digital mode
    modeColors[1] = ledColors[colorIndex];  // Analog mode
  }

  // Always keep Pip-Boy in green
  modeColors[2] = ledColors[COLOR_GREEN];
}

// Cycle through available LED colors
void cycleLedColor() {
  // Cycle through predefined colors
  currentLedColor = (currentLedColor + 1) % COLOR_TOTAL;

  // Update the mode colors based on the new color
  updateModeColorsFromLedColor(currentLedColor);
}

// Get the current second ring color based on mode
uint16_t getCurrentSecondRingColor() {
  // Return the TFT color value for the current mode
  return modeColors[currentMode].tft_color;
}

// Return the current LED color index
int getCurrentLedColor() {
  return currentLedColor;
}

#endif  // THEME_MANAGER_H
