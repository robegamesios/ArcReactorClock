/*
 * led_controls.h - Functions for controlling the LED ring
 * For Multi-Mode Digital Clock project
 */

#ifndef LED_CONTROLS_H
#define LED_CONTROLS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// LED ring brightness settings
int led_ring_brightness = 10;      // Normal brightness (0-255)
int led_ring_brightness_flash = 250; // Flash brightness (0-255)

// LED color settings for each mode
struct LEDColors {
  int r;
  int g;
  int b;
};

// Mode colors definitions
LEDColors modeColors[3] = {
  {0, 20, 255},   // Blue for Arc Reactor digital
  {0, 20, 255},   // Blue for Arc Reactor analog
  {0, 255, 50}    // Green for Pip-Boy mode
};

// External references
extern Adafruit_NeoPixel pixels;
extern int currentMode;

// Function declarations
void greenLight();
void blueLight();
void flashEffect();
void updateLEDs();

// LED ring functions
void greenLight() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to Pip-Boy green color
  for(int i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(
      modeColors[2].r, 
      modeColors[2].g, 
      modeColors[2].b
    ));
  }
  pixels.show();
}

void blueLight() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to Arc Reactor blue color
  for(int i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(
      modeColors[0].r, 
      modeColors[0].g, 
      modeColors[0].b
    ));
  }
  pixels.show();
}

void flashEffect() {
  pixels.setBrightness(led_ring_brightness_flash);
  // Set all pixels to white
  for(int i=0; i<pixels.numPixels(); i++) {
    pixels.setPixelColor(i, pixels.Color(250, 250, 250));
  }
  pixels.show();
  
  // Fade down brightness
  for (int i=led_ring_brightness_flash; i>10; i--) {
    pixels.setBrightness(i);
    pixels.show();
    delay(7);
  }
  
  // Return to appropriate color for current mode
  updateLEDs();
}

void updateLEDs() {
  // Set colors based on current mode
  switch (currentMode) {
    case 0: // MODE_ARC_DIGITAL
    case 1: // MODE_ARC_ANALOG
      blueLight();
      break;
      
    case 2: // MODE_PIPBOY
      greenLight();
      break;
  }
}

#endif // LED_CONTROLS_H
