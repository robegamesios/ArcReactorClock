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

// LED ring brightness settings
extern int led_ring_brightness;
extern int led_ring_brightness_flash;

// Function declarations for LED control
void updateLEDs();
void flashEffect();

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

#endif  // LED_CONTROLS_H
