/*
 * theme_manager.h - Theme management system
 * For Multi-Mode Digital Clock project
 * Allows custom LED colors for all modes with weather-based color changes
 */

#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// Clock modes
#define MODE_ARC_DIGITAL 0
#define MODE_ARC_ANALOG 1
#define MODE_PIPBOY 2
#define MODE_GIF_DIGITAL 3
#define MODE_WEATHER 4
#define MODE_APPLE_RINGS 5
#define MODE_TOTAL 6

// LED color IDs
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

// Weather-specific colors for automatic ambient LED coloring
#define COLOR_FREEZING_BLUE 12  // Icy blue for freezing temperatures
#define COLOR_COLD_BLUE 13      // Cold blue for cold temperatures
#define COLOR_COOL_CYAN 14      // Cyan for cool temperatures
#define COLOR_COMFORT_GREEN 15  // Green for comfortable temperatures
#define COLOR_WARM_YELLOW 16    // Yellow for warm temperatures
#define COLOR_HOT_ORANGE 17     // Orange for hot temperatures
#define COLOR_VERY_HOT_RED 18   // Bright red for very hot temperatures
#define COLOR_STORM_PURPLE 19   // Purple for storm conditions
#define COLOR_RAIN_BLUE 20      // Deep blue for rain
#define COLOR_SNOW_WHITE 21     // Bright white for snow
#define COLOR_FOG_GRAY 22       // Gray for fog/mist conditions

#define COLOR_TOTAL 23  // Total number of available colors

// Extended LED color structure with name
struct LEDColorDefinition {
  int r;
  int g;
  int b;
  uint16_t tft_color;
  const char* name;
};

// Predefined LED colors array - now with names (R, G, B, TFT color, name) format
LEDColorDefinition ledColors[COLOR_TOTAL] = {
  // Original colors
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
  { 255, 255, 255, 0xFFFF, "White" },

  // Weather condition colors
  { 150, 230, 255, 0x9FFF, "Freezing" },    // Icy blue
  { 40, 100, 255, 0x257F, "Cold" },         // Cold blue
  { 0, 200, 220, 0x07DD, "Cool" },          // Turquoise
  { 20, 220, 120, 0x17E0, "Comfortable" },  // Pleasant green
  { 255, 240, 50, 0xFFA0, "Warm" },         // Warm yellow
  { 255, 150, 0, 0xFC60, "Hot" },           // Orange
  { 255, 50, 0, 0xFB00, "Very Hot" },       // Hot red
  { 130, 0, 220, 0x801B, "Storm" },         // Storm purple
  { 20, 80, 200, 0x14CD, "Rain" },          // Rain blue
  { 240, 240, 255, 0xF7FF, "Snow" },        // Snow white
  { 140, 140, 160, 0x8DB4, "Fog" }          // Fog/mist gray
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

// Set theme based on background filename
void setThemeFromFilename(const char* filename) {
  // No color changes - user preferences are preserved
}

// Update LED color
void updateModeColorsFromLedColor(int colorIndex) {
  if (colorIndex < 0 || colorIndex >= COLOR_TOTAL) {
    colorIndex = COLOR_BLUE;  // Default to blue if invalid
  }

  currentLedColor = colorIndex;
}

// Get color name
const char* getColorName(int colorIndex) {
  if (colorIndex >= 0 && colorIndex < COLOR_TOTAL) {
    return ledColors[colorIndex].name;
  }
  return "Unknown Color";
}

// Cycle through available LED colors
void cycleLedColor() {
  // Skip the weather-specific colors in manual cycling
  currentLedColor = (currentLedColor + 1) % COLOR_TOTAL;

  // Skip the weather colors when cycling manually
  if (currentLedColor >= COLOR_FREEZING_BLUE && currentLedColor <= COLOR_FOG_GRAY) {
    currentLedColor = COLOR_IRONMAN_RED;  // Wrap back to first color
  }

  // Show color name overlay - function defined in led_controls.h
  showColorNameOverlay();
}

// Get the current second ring color based on mode
uint16_t getCurrentSecondRingColor() {
  // For Pip-Boy mode, we'll still use green for the SCREEN elements
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
