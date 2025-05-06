/*
 * gif_digital.h - GIF Background Digital Clock Mode
 * For Multi-Mode Digital Clock project
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

extern int CLOCK_VERTICAL_OFFSET;

// For GIF Digital mode
int prevHoursGif = -1, prevMinutesGif = -1, prevSecondsGif = -1;
bool prevColonStateGif = false;
bool showColonGif = true;

// GIF background handling
AnimatedGIF gifDigitalClock;
uint8_t *gifDigitalBuffer = NULL;
int gifDigitalSize = 0;

#define CYAN_COLOR 0x07FF
#define TEXT_BACKGROUND_COLOR 0x0001

// Function prototypes
void GIFDrawDigital(GIFDRAW *pDraw);
bool displayGIFDigitalBackground(const char *filename);
void drawGifDigitalBackground(const char *gifFilename);
void updateGifDigitalTime();
void updateGifDigitalColon();
void resetGifDigitalVariables();
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
  Serial.print("GIF Digital: Loading background: ");
  Serial.println(filename);

  // Skip vaultboy.gif - this is reserved for Pip-Boy mode
  if (strstr(filename, "vaultboy.gif") != NULL) {
    Serial.println("Skipping vaultboy.gif - this is for Pip-Boy mode");
    return false;
  }

  // Check if file exists
  if (!SPIFFS.exists(filename)) {
    Serial.print("ERROR: GIF file not found: ");
    Serial.println(filename);
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
    Serial.print("Failed to open GIF file: ");
    Serial.println(filename);
    return false;
  }

  // Get file size
  gifDigitalSize = f.size();
  if (gifDigitalSize == 0) {
    Serial.println("GIF file is empty");
    f.close();
    return false;
  }

  // Allocate memory for the GIF data
  gifDigitalBuffer = (uint8_t *)malloc(gifDigitalSize);
  if (gifDigitalBuffer == NULL) {
    Serial.println("Failed to allocate memory for GIF");
    f.close();
    gifDigitalSize = 0;
    return false;
  }

  // Read the file into the buffer
  size_t bytesRead = f.read(gifDigitalBuffer, gifDigitalSize);
  f.close();

  if (bytesRead != gifDigitalSize) {
    Serial.println("Failed to read entire GIF file");
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
    return false;
  }

  // Initialize the GIF decoder
  Serial.println("Initializing GIF decoder...");
  gifDigitalClock.begin(GIF_PALETTE_RGB565_LE);

  // Debug GIF information
  Serial.print("GIF byte order: ");
  Serial.println("RGB565_LE (little endian)");

  // Open the GIF from the buffer with our custom draw callback
  if (!gifDigitalClock.open(gifDigitalBuffer, gifDigitalSize, GIFDrawDigital)) {
    Serial.println("Failed to decode GIF file");
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
    return false;
  }

  // Display the first frame
  if (!gifDigitalClock.playFrame(true, NULL)) {
    Serial.println("Failed to display first GIF frame");
    gifDigitalClock.close();
    free(gifDigitalBuffer);
    gifDigitalBuffer = NULL;
    gifDigitalSize = 0;
    return false;
  }

  Serial.println("SUCCESS: GIF loaded and first frame displayed!");
  return true;
}

// Draw the GIF background with the specified file
void drawGifDigitalBackground(const char *gifFilename) {
  // Clear the display
  tft.fillScreen(TFT_BLACK);

  // Try to load GIF background using the provided filename
  if (!displayGIFDigitalBackground(gifFilename)) {
    Serial.println("No GIF background found or error loading, using plain background");
  }

  Serial.print("Clock vertical position offset: ");
  Serial.println(CLOCK_VERTICAL_OFFSET);
}

// Reset variables to force redraw of digital clock
void resetGifDigitalVariables() {
  prevHoursGif = -1;
  prevMinutesGif = -1;
  prevSecondsGif = -1;
  prevColonStateGif = false;
  showColonGif = true;
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
    Serial.println("GIF Digital resources cleaned up");
  }
}

// Update digital time display
void updateGifDigitalTime() {
  // Only update parts that have changed
  bool hoursChanged = (hours != prevHoursGif);
  bool minutesChanged = (minutes != prevMinutesGif);
  bool secondsChanged = (seconds != prevSecondsGif);
  bool colonChanged = (showColonGif != prevColonStateGif);

  // Only update the display if the time or colon state has changed
  if (hoursChanged || minutesChanged || secondsChanged || colonChanged) {
    // Create backgrounds for text that preserve most of the underlying image
    tft.setTextColor(CYAN_COLOR, TEXT_BACKGROUND_COLOR);

    // Handle seconds update - at the top for symmetry
    if (seconds != prevSecondsGif) {
      // Format seconds with leading zero if needed
      char timeStr[6];
      if (seconds < 10) {
        sprintf(timeStr, "0%d", seconds);
      } else {
        sprintf(timeStr, "%d", seconds);
      }

      // Draw seconds with semi-transparent background
      tft.setTextSize(2);
      tft.setCursor(screenCenterX - 10, screenCenterY - 40 + CLOCK_VERTICAL_OFFSET);
      tft.print(timeStr);
    }

    // Handle hours update
    if (hours != prevHoursGif || (minutes != prevMinutesGif && hours < 10)) {
      // Format hours with leading zero if needed
      char timeStr[6];
      int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
      if (displayHours < 10) {
        sprintf(timeStr, "0%d", displayHours);
      } else {
        sprintf(timeStr, "%d", displayHours);
      }

      // Draw hours text with semi-transparent background
      tft.setTextSize(4);
      tft.setCursor(screenCenterX - 58, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
      tft.print(timeStr);
    }

    // Handle colon update (only if colon state changed)
    if (showColonGif != prevColonStateGif) {
      // Colon position - apply vertical offset
      int colonX = screenCenterX - 15;
      int colonY = screenCenterY - 25 + CLOCK_VERTICAL_OFFSET;
      int colonWidth = 25;
      int colonHeight = 45;

      // Either draw the colon or clear its area by redrawing background
      if (showColonGif) {
        tft.setTextSize(4);
        tft.setCursor(screenCenterX - 10, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
        tft.print(":");
      } else {
        // When colon needs to be hidden, draw a small rect with the background color
        tft.fillRect(colonX, colonY, colonWidth, colonHeight, TEXT_BACKGROUND_COLOR);
      }
    }

    // Handle minutes update
    if (minutes != prevMinutesGif) {
      // Format minutes with leading zero if needed
      char timeStr[6];
      if (minutes < 10) {
        sprintf(timeStr, "0%d", minutes);
      } else {
        sprintf(timeStr, "%d", minutes);
      }

      // Draw minutes text with semi-transparent background - apply vertical offset
      tft.setTextSize(4);
      tft.setCursor(screenCenterX + 15, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
      tft.print(timeStr);
    }

    // Handle AM/PM indicator (only in 12-hour mode and if hour changed)
    if (!is24Hour && (hours != prevHoursGif || (prevHoursGif == -1))) {
      struct tm timeinfo;
      bool isPM = false;

      if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
        isPM = (timeinfo.tm_hour >= 12);
      } else {
        // For manual time, determine AM/PM based on hours
        isPM = (hours >= 12);
      }

      // Draw AM/PM indicator with semi-transparent background - apply vertical offset
      tft.setTextSize(2);
      tft.setCursor(screenCenterX - 10, screenCenterY + 20 + CLOCK_VERTICAL_OFFSET);
      if (isPM) {
        tft.println("PM");
      } else {
        tft.println("AM");
      }
    }

    // Save current state for next comparison
    prevHoursGif = hours;
    prevMinutesGif = minutes;
    prevSecondsGif = seconds;
    prevColonStateGif = showColonGif;
  }
}

// For blinking colon in digital mode
void updateGifDigitalColon() {
  // Toggle colon state
  bool oldColonState = showColonGif;
  showColonGif = !showColonGif;

  // Only call update if colon state changed
  if (oldColonState != showColonGif) {
    // Position for colon - apply vertical offset
    int colonX = screenCenterX - 15;
    int colonY = screenCenterY - 25 + CLOCK_VERTICAL_OFFSET;
    int colonWidth = 25;
    int colonHeight = 45;

    if (showColonGif) {
      // Draw colon with semi-transparent background
      tft.setTextColor(CYAN_COLOR, TEXT_BACKGROUND_COLOR);
      tft.setTextSize(4);
      tft.setCursor(screenCenterX - 10, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
      tft.print(":");
    } else {
      // Clear the colon with background color
      tft.fillRect(colonX, colonY, colonWidth, colonHeight, TEXT_BACKGROUND_COLOR);
    }

    // Update the state tracking
    prevColonStateGif = showColonGif;
  }
}

#endif  // GIF_DIGITAL_H
