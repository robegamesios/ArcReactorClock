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
// When adding a new color, make sure to update this to add a new #define 
// along with LEDColorDefinition ledColors[COLOR_TOTAL]
#define COLOR_IRONMAN_RED 0
#define COLOR_SPIDERMAN_RED 1
#define COLOR_RED 2
#define COLOR_CAPTAIN_BLUE 3
#define COLOR_BLUE 4
#define COLOR_CYAN 5
#define COLOR_HULK_GREEN 6
#define COLOR_GREEN 7
#define COLOR_PURPLE 8
#define COLOR_IRONMAN_GOLD 9
#define COLOR_YELLOW 10
#define COLOR_WHITE 11
#define COLOR_TOTAL 12

// Extended LED color structure with name
struct LEDColorDefinition {
  int r;
  int g;
  int b;
  uint16_t tft_color;
  const char* name;
};

// Predefined LED colors array - now with names (R, G, B, name) format
LEDColorDefinition ledColors[COLOR_TOTAL] = {
  { 180, 20, 5, 0xB081, "Iron Man Red" },
  { 220, 0, 10, 0xE004, "Spiderman Red" },
  { 255, 0, 0, 0xF800, "Bright Red" },
  { 30, 60, 150, 0x1B0C, "Cap America Blue" },
  { 0, 20, 255, 0x051F, "Arc Reactor Blue" },
  { 0, 255, 255, 0x07FF, "Cyan" },
  { 40, 130, 10, 0x2680, "Hulk Green" },
  { 0, 255, 50, 0x07E0, "Bright Green" },
  { 180, 0, 255, 0xC01F, "Purple" },
  { 200, 140, 0, 0xCA00, "Iron Man Gold" },
  { 255, 255, 0, 0xFFE0, "Yellow" },
  { 255, 255, 255, 0xFFFF, "White" }
};

// Track current LED color
int currentLedColor = COLOR_BLUE;  // Current LED color index

// External references (to be defined in main sketch)
extern int currentMode;
extern TFT_eSPI tft;
extern int screenCenterX;
extern int screenCenterY;
extern bool needClockRefresh;

// Forward declarations
void setThemeFromFilename(const char* filename);
void updateModeColorsFromLedColor(int colorIndex);
uint16_t getCurrentSecondRingColor();
void cycleLedColor();
void showColorNameOverlay();
const char* getColorName(int colorIndex);

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

// Get color name directly from the structure
const char* getColorName(int colorIndex) {
  if (colorIndex >= 0 && colorIndex < COLOR_TOTAL) {
    return ledColors[colorIndex].name;
  }
  return "Unknown Color";
}

// Cycle through available LED colors
void cycleLedColor() {
  // Cycle through predefined colors
  currentLedColor = (currentLedColor + 1) % COLOR_TOTAL;

  // Show color name overlay - function defined in led_controls.h
  showColorNameOverlay();
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
