/*
 * gif_digital.h - GIF Background Mode (No Clock Overlay)
 * For Multi-Mode Digital Clock project
 * Modified to remove clock display functionality
 */

#ifndef GIF_DIGITAL_H
#define GIF_DIGITAL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include <AnimatedGIF.h>
#include "utils.h"
#include "led_controls.h"

// GIF background handling
AnimatedGIF gifDigitalClock;
uint8_t *gifDigitalBuffer = NULL;
int gifDigitalSize = 0;

// Function prototypes
void GIFDrawDigital(GIFDRAW *pDraw);
bool displayGIFDigitalBackground(const char *filename);
void drawGifDigitalBackground(const char *gifFilename);
void updateGifDigitalBackground();
void cleanupGifDigitalMode();

// GIF drawing callback for digital clock mode with color correction
void GIFDrawDigital(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > 240)  // Limit to screen width
    iWidth = 240;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;  // current line

  // Calculate centering offsets for 240x240 screen
  int xOffset = (240 - pDraw->iWidth) / 2;
  int yOffset = (240 - pDraw->iHeight) / 2;

  // Apply the vertical offset to the Y position
  int centeredY = y + yOffset;

  // Skip if outside display area
  if (centeredY >= 240 || centeredY < 0 || (xOffset + pDraw->iX) >= 240)
    return;

  s = pDraw->pPixels;

  // Process the line of pixels
  for (x = 0; x < iWidth; x++) {
    if (s[x] == pDraw->ucTransparent) {
      usTemp[x] = TFT_BLACK;  // Force transparent pixels to black
    } else {
      // Swap bytes to fix colors
      uint16_t color = usPalette[s[x]];
      usTemp[x] = (color >> 8) | (color << 8);
    }
  }
  d = usTemp;

  // Draw the current line of the GIF at the centered position
  tft.pushImage(xOffset + pDraw->iX, centeredY, iWidth, 1, d);
}

// Load and display a GIF background
bool displayGIFDigitalBackground(const char *filename) {
  // Skip vaultboy.gif - this is reserved for Pip-Boy mode
  if (strstr(filename, "vaultboy.gif") != NULL) {
    return false;
  }

  // Check if file exists
  if (!SPIFFS.exists(filename)) {
    return false;
  }

  // Clean up previous GIF if any
  if (gifDigitalBuffer != NULL) {
    gifDigitalClock.close();
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
  }

  // Extract filename for theme detection
  String fullPath = String(filename);
  String justFilename = fullPath;
  int lastSlash = fullPath.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < fullPath.length() - 1) {
    justFilename = fullPath.substring(lastSlash + 1);
  }

  // Set theme based on filename
  setThemeFromFilename(justFilename.c_str());

  // Open the file
  File f = SPIFFS.open(filename, "r");
  if (!f) {
    return false;
  }

  // Get file size
  gifDigitalSize = f.size();
  if (gifDigitalSize == 0) {
    f.close();
    return false;
  }

  // Allocate memory for the GIF data
  gifDigitalBuffer = (uint8_t *)malloc(gifDigitalSize);
  if (gifDigitalBuffer == NULL) {
    f.close();
    gifDigitalSize = 0;
    return false;
  }

  // Read the file into the buffer
  size_t bytesRead = f.read(gifDigitalBuffer, gifDigitalSize);
  f.close();

  if (bytesRead != gifDigitalSize) {
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
    return false;
  }

  // Initialize the GIF decoder
  gifDigitalClock.begin(GIF_PALETTE_RGB565_LE);

  // Open the GIF from the buffer with our custom draw callback
  if (!gifDigitalClock.open(gifDigitalBuffer, gifDigitalSize, GIFDrawDigital)) {
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
    return false;
  }

  // Display the first frame
  if (!gifDigitalClock.playFrame(true, NULL)) {
    gifDigitalClock.close();
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
    return false;
  }

  return true;
}

// Draw the GIF background with the specified file
void drawGifDigitalBackground(const char *gifFilename) {
  // Clear the display
  tft.fillScreen(TFT_BLACK);

  // Try to load GIF background using the provided filename
  if (!displayGIFDigitalBackground(gifFilename)) {
    // Fallback to plain background if GIF loading fails
  }
}

// Update the GIF animation - advance to next frame
void updateGifDigitalBackground() {
  // Check if GIF exists and is loaded
  if (gifDigitalBuffer != NULL && gifDigitalSize > 0) {
    // Try to play the next frame
    if (!gifDigitalClock.playFrame(true, NULL)) {
      // End of animation, reset to beginning
      gifDigitalClock.reset();
    }
  }
}

// Clean up resources when switching away from GIF Digital mode
void cleanupGifDigitalMode() {
  if (gifDigitalBuffer != NULL) {
    gifDigitalClock.close();
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
  }
}

#endif  // GIF_DIGITAL_H
