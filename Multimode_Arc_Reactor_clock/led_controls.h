/*
 * led_controls.h - Functions for controlling the LED ring
 * For Multi-Mode Digital Clock project
 */

#ifndef LED_CONTROLS_H
#define LED_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// First include theme_persistence.h AFTER we define constants, not before
// This prevents circular dependency issues

// Character-based theme identifiers
// These need to be lowercase as we're doing case-insensitive matching
const char* THEME_IRONMAN = "ironman";
const char* THEME_HULK = "hulk";
const char* THEME_CAPTAIN = "captain";
const char* THEME_THOR = "thor";
const char* THEME_BLACK_WIDOW = "widow";
const char* THEME_SPIDERMAN = "spiderman";  // Added Spiderman theme

// NOW we can include theme_persistence.h
#include "theme_persistence.h"

// LED ring brightness settings
int led_ring_brightness = 100;        // Normal brightness (0-255)
int led_ring_brightness_flash = 250;  // Flash brightness (0-255)

// LED color settings for each mode
struct LEDColors {
  int r;
  int g;
  int b;
  uint16_t tft_color;  // Added TFT color value for easy use with the display
};

// Mode colors definitions (default values - can be changed dynamically)
LEDColors modeColors[3] = {
  { 0, 20, 255, 0x051F },  // Blue for Arc Reactor digital with TFT color
  { 0, 20, 255, 0x051F },  // Blue for Arc Reactor analog with TFT color (matching digital)
  { 0, 255, 50, 0x07E0 }   // Green for Pip-Boy mode with TFT color
};

// Track current theme
String currentThemeName = THEME_IRONMAN;  // Default theme

// External references
extern Adafruit_NeoPixel pixels;
extern int currentMode;

// Function declarations
void greenLight();
void blueLight();
void flashEffect();
void updateLEDs();
void setThemeFromFilename(const char* filename);
uint16_t getCurrentSecondRingColor();
String getCurrentThemeName();
void setThemeFromName(String themeName);

// Helper function to check if a saved theme matches one of our known themes
bool checkSavedThemeAgainstKnown() {
  // Check each theme to see if it matches the saved hash
  if (isThemeHashMatch(THEME_IRONMAN)) {
    setThemeFromName(THEME_IRONMAN);
    return true;
  } else if (isThemeHashMatch(THEME_HULK)) {
    setThemeFromName(THEME_HULK);
    return true;
  } else if (isThemeHashMatch(THEME_CAPTAIN)) {
    setThemeFromName(THEME_CAPTAIN);
    return true;
  } else if (isThemeHashMatch(THEME_THOR)) {
    setThemeFromName(THEME_THOR);
    return true;
  } else if (isThemeHashMatch(THEME_BLACK_WIDOW)) {
    setThemeFromName(THEME_BLACK_WIDOW);
    return true;
  } else if (isThemeHashMatch(THEME_SPIDERMAN)) {
    setThemeFromName(THEME_SPIDERMAN);
    return true;
  }

  return false;  // No match found
}

// Configure theme based on JPEG filename
void setThemeFromName(String themeName) {
  if (themeName.length() == 0) {
    return;  // Don't change anything if no theme name provided
  }

  // Create a fake filename that starts with the theme name
  String fakeFilename = "/" + themeName + "00.jpg";

  // Use the existing function to set the theme
  setThemeFromFilename(fakeFilename.c_str());
}

// Return the current theme name
String getCurrentThemeName() {
  return currentThemeName;
}

// Get the current second ring color based on mode
uint16_t getCurrentSecondRingColor() {
  // Return the TFT color value for the current mode
  return modeColors[currentMode].tft_color;
}

// LED ring functions
void greenLight() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to Pip-Boy green color
  for (int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(
                              modeColors[2].r,
                              modeColors[2].g,
                              modeColors[2].b));
  }
  pixels.show();
}

void blueLight() {
  pixels.setBrightness(led_ring_brightness);
  // Use the current mode's color
  int modeIndex = (currentMode == 0) ? 0 : 1;  // Use different colors for digital vs analog
  for (int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(
                              modeColors[modeIndex].r,
                              modeColors[modeIndex].g,
                              modeColors[modeIndex].b));
  }
  pixels.show();
}

void flashEffect() {
  pixels.setBrightness(led_ring_brightness_flash);
  // Set all pixels to white
  for (int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(250, 250, 250));
  }
  pixels.show();

  // Fade down brightness
  for (int i = led_ring_brightness_flash; i > 10; i--) {
    pixels.setBrightness(i);
    pixels.show();
    delay(8);
  }

  // Return to appropriate color for current mode
  updateLEDs();
}

void updateLEDs() {
  // Set colors based on current mode
  switch (currentMode) {
    case 0:  // MODE_ARC_DIGITAL
    case 1:  // MODE_ARC_ANALOG
      blueLight();
      break;

    case 2:  // MODE_PIPBOY
      greenLight();
      break;
  }
}

#endif  // LED_CONTROLS_H
