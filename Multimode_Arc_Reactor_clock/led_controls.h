/*
 * led_controls.h - Functions for controlling the LED ring
 * For Multi-Mode Digital Clock project
 */

#ifndef LED_CONTROLS_H
#define LED_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// LED ring brightness settings
int led_ring_brightness = 100;        // Normal brightness (0-255)
int led_ring_brightness_flash = 250;  // Flash brightness (0-255)

// LED color settings for each mode
struct LEDColors {
  int r;
  int g;
  int b;
  uint16_t tft_color; // Added TFT color value for easy use with the display
};

// Mode colors definitions (default values - can be changed dynamically)
LEDColors modeColors[3] = {
  { 0, 20, 255, 0x051F },   // Blue for Arc Reactor digital with TFT color
  { 0, 20, 255, 0x051F },   // Blue for Arc Reactor analog with TFT color (matching digital)
  { 0, 255, 50, 0x07E0 }    // Green for Pip-Boy mode with TFT color
};

// Character-based theme identifiers
// These need to be lowercase as we're doing case-insensitive matching
#define THEME_IRONMAN "ironman"
#define THEME_HULK "hulk"
#define THEME_CAPTAIN "captain"
#define THEME_THOR "thor"
#define THEME_BLACK_WIDOW "widow"
#define THEME_SPIDERMAN "spiderman"  // Added Spiderman theme

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

// Configure theme based on JPEG filename
void setThemeFromFilename(const char* filename) {
  // Create a debug message showing exactly what filename we're checking
  Serial.print("Setting theme from filename: ");
  Serial.println(filename);
  
  // Create a clean version of the filename (lowercase, no path)
  String fname = String(filename);
  fname.toLowerCase();
  
  // Remove leading slash if present
  if (fname.startsWith("/")) {
    fname = fname.substring(1);
  }
  
  // Debug the cleaned filename
  Serial.print("Cleaned filename for theme check: ");
  Serial.println(fname);
  
  // Default is blue theme for both modes
  // Apply default theme first
  modeColors[0] = { 0, 20, 255, 0x051F };   // Blue digital
  modeColors[1] = { 0, 20, 255, 0x051F };   // Blue analog
  
  // Let's do more explicit theme debugging
  if (fname.startsWith(THEME_IRONMAN)) {
    // Iron Man theme - Blue for both
    modeColors[0] = { 0, 20, 255, 0x051F };   // Blue digital
    modeColors[1] = { 0, 20, 255, 0x051F };   // Blue analog (matching digital)
    Serial.println("→ Matched Iron Man theme (blue)");
  }
  else if (fname.startsWith(THEME_HULK)) {
    // Hulk theme - Green for both
    modeColors[0] = { 0, 255, 50, 0x07E0 };   // Green digital
    modeColors[1] = { 0, 255, 50, 0x07E0 };   // Green analog
    Serial.println("→ Matched Hulk theme (green)");
  }
  else if (fname.startsWith(THEME_CAPTAIN)) {
    // Captain America theme - Blue for both
    modeColors[0] = { 0, 50, 255, 0x037F };   // Blue digital
    modeColors[1] = { 0, 50, 255, 0x037F };   // Blue analog (matching digital)
    Serial.println("→ Matched Captain America theme (blue)");
  }
  else if (fname.startsWith(THEME_THOR)) {
    // Thor theme - Yellow for both
    modeColors[0] = { 255, 255, 0, 0xFFE0 };  // Yellow digital
    modeColors[1] = { 255, 255, 0, 0xFFE0 };  // Yellow analog
    Serial.println("→ Matched Thor theme (yellow)");
  }
  else if (fname.startsWith(THEME_BLACK_WIDOW)) {
    // Black Widow theme - Red for both
    modeColors[0] = { 255, 0, 0, 0xF800 };    // Red digital
    modeColors[1] = { 255, 0, 0, 0xF800 };    // Red analog
    Serial.println("→ Matched Black Widow theme (red)");
  }
  else if (fname.startsWith(THEME_SPIDERMAN)) {
    // Spiderman theme - Red for both
    modeColors[0] = { 255, 0, 0, 0xF800 };    // Red digital
    modeColors[1] = { 255, 0, 0, 0xF800 };    // Red analog
    Serial.println("→ Matched Spiderman theme (red)");
  }
  else {
    // No theme matched, use default
    Serial.println("→ No specific theme matched, using default (blue)");
  }
  
  // Print the selected colors for debugging
  Serial.print("Selected colors - Digital: RGB(");
  Serial.print(modeColors[0].r);
  Serial.print(",");
  Serial.print(modeColors[0].g);
  Serial.print(",");
  Serial.print(modeColors[0].b);
  Serial.print("), Analog: RGB(");
  Serial.print(modeColors[1].r);
  Serial.print(",");
  Serial.print(modeColors[1].g);
  Serial.print(",");
  Serial.print(modeColors[1].b);
  Serial.println(")");
  
  // Update the LED colors immediately
  updateLEDs();
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
