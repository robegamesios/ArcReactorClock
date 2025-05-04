/*
 * led_controls.h - Functions for controlling the LED ring
 * For Multi-Mode Digital Clock project
 * 
 * REVISED VERSION - Allows custom LED colors for all modes
 */

#ifndef LED_CONTROLS_H
#define LED_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// Include theme manager for access to theme definitions and colors
#include "theme_manager.h"

// External references
extern Adafruit_NeoPixel pixels;
extern int currentMode;
extern bool needClockRefresh;
extern TFT_eSPI tft;
extern int screenCenterX;
extern int screenCenterY;

// LED ring brightness settings
extern int led_ring_brightness;
extern int led_ring_brightness_flash;

// Variables for color name overlay
unsigned long lastColorChangeTime = 0;
bool showColorName = false;

// Function declarations for LED control
void updateLEDs();
void flashEffect();
void showColorNameOverlay();
void checkColorNameTimeout();

// Update LED ring based on current settings
void updateLEDs() {
  pixels.setBrightness(led_ring_brightness);

  // Use the user-selected color for all modes
  for (int i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(
                              ledColors[currentLedColor].r,
                              ledColors[currentLedColor].g,
                              ledColors[currentLedColor].b));
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

// Display the color name overlay
void showColorNameOverlay() {
  showColorName = true;
  lastColorChangeTime = millis();

  // Get color name directly from the structure
  const char* colorName = ledColors[currentLedColor].name;

  // Create a semi-transparent background for text
  int textWidth = strlen(colorName) * 12;  // Approximate width
  int rectX = screenCenterX - (textWidth / 2) - 10;
  int rectY = screenCenterY - 40;
  int rectWidth = textWidth + 20;
  int rectHeight = 30;

  // Draw background
  tft.fillRoundRect(rectX, rectY, rectWidth, rectHeight, 5, TFT_BLACK);
  tft.drawRoundRect(rectX, rectY, rectWidth, rectHeight, 5, ledColors[currentLedColor].tft_color);

  // Draw text
  tft.setTextSize(2);
  tft.setTextColor(ledColors[currentLedColor].tft_color);
  tft.setCursor(screenCenterX - (textWidth / 2), screenCenterY - 35);
  tft.println(colorName);
}

// Check if color name overlay should be hidden
void checkColorNameTimeout() {
  if (showColorName && (millis() - lastColorChangeTime > 2000)) {
    showColorName = false;

    // Redraw the current display to clear the overlay
    needClockRefresh = true;  // This will trigger a redraw on the next clock update
  }
}

#endif  // LED_CONTROLS_H
