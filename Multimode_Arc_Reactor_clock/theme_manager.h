/*
 * theme_manager.h - Simplified theme management system
 * For Multi-Mode Digital Clock project
 * 
 * REVISED VERSION - Allows custom LED colors for all modes
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

// Simplified function that just logs the background being loaded
void setThemeFromFilename(const char* filename) {
  Serial.print("Loading background: ");
  Serial.println(filename);
  // No color changes - user preferences are preserved
}

// Simplified function to update LED color
void updateModeColorsFromLedColor(int colorIndex) {
  if (colorIndex < 0 || colorIndex >= COLOR_TOTAL) {
    colorIndex = COLOR_BLUE;  // Default to blue if invalid
  }

  currentLedColor = colorIndex;
}

// Cycle through available LED colors
void cycleLedColor() {
  // Cycle through predefined colors
  currentLedColor = (currentLedColor + 1) % COLOR_TOTAL;
}

// Get the current second ring color based on mode
uint16_t getCurrentSecondRingColor() {
  // For Pip-Boy mode, we'll still use green for the SCREEN elements
  // (since green is characteristic of the Pip-Boy interface)
  if (currentMode == MODE_PIPBOY) {
    return ledColors[COLOR_GREEN].tft_color;
  }
  // For other modes, use the current user-selected LED color
  else {
    return ledColors[currentLedColor].tft_color;
  }
}

// Return the current LED color index
int getCurrentLedColor() {
  return currentLedColor;
}

#endif  // THEME_MANAGER_H