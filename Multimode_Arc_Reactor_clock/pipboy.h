/*
 * pipboy.h - Pip-Boy 3000 Clock Mode
 * For Multi-Mode Digital Clock project
 */

#ifndef PIPBOY_H
#define PIPBOY_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "utils.h"
#include <AnimatedGIF.h>
#include <FS.h>  // Include FS for File class

// Function prototypes
void drawPipBoyInterface();
void updatePipBoyTime();
void updatePipBoyGif();
void cleanupPipBoyMode();
void GIFDraw(GIFDRAW *pDraw);
bool loadAndInitGIF(const char *gifPath);

// Global variables
AnimatedGIF gif;
const int figureX = 75;     // Position for Vault Boy figure
uint8_t *gifBuffer = NULL;  // Buffer to hold GIF data
int gifSize = 0;

// GIF drawing function for the AnimatedGIF library
// GIF drawing function for the AnimatedGIF library
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > tft.width())
    iWidth = tft.width();

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;  // current line

  // Skip the first few rows to remove extra space at the top if needed
  if (y < 5) return;

  s = pDraw->pPixels;

  for (x = 0; x < iWidth; x++) {
    if (s[x] == pDraw->ucTransparent) {
      usTemp[x] = PIP_BLACK;  // Force transparent pixels to black
    } else {
      // Apply byte swapping to fix colors
      uint16_t color = usPalette[s[x]];
      usTemp[x] = (color >> 8) | (color << 8);
    }
  }
  d = usTemp;

  // Position the GIF
  tft.pushImage(pDraw->iX + figureX - 20, (y - 5) + 80, iWidth, 1, d);
}

// Improved GIF loading function
bool loadAndInitGIF(const char *gifPath) {
  // Clear any existing GIF resources
  if (gifBuffer != NULL) {
    gif.close();
    free(gifBuffer);
    gifBuffer = NULL;
    gifSize = 0;
  }

  // Check if the file exists
  if (!SPIFFS.exists(gifPath)) {
    Serial.print("GIF file not found: ");
    Serial.println(gifPath);
    return false;
  }

  // Open the file
  File f = SPIFFS.open(gifPath, "r");
  if (!f) {
    Serial.print("Failed to open GIF file: ");
    Serial.println(gifPath);
    return false;
  }

  // Get file size
  gifSize = f.size();
  if (gifSize == 0) {
    Serial.println("GIF file is empty");
    f.close();
    return false;
  }

  // Allocate memory for the GIF data
  gifBuffer = (uint8_t *)malloc(gifSize);
  if (gifBuffer == NULL) {
    Serial.println("Failed to allocate memory for GIF");
    f.close();
    gifSize = 0;
    return false;
  }

  // Read the file into the buffer
  size_t bytesRead = f.read(gifBuffer, gifSize);
  f.close();

  if (bytesRead != gifSize) {
    Serial.println("Failed to read entire GIF file");
    free(gifBuffer);
    gifBuffer = NULL;
    gifSize = 0;
    return false;
  }

  // Initialize the GIF decoder
  gif.begin(GIF_PALETTE_RGB565_LE);

  // Open the GIF from the buffer
  if (!gif.open(gifBuffer, gifSize, GIFDraw)) {
    Serial.println("Failed to decode GIF file");
    free(gifBuffer);
    gifBuffer = NULL;
    gifSize = 0;
    return false;
  }

  // Successfully loaded and initialized the GIF
  Serial.println("GIF loaded and initialized successfully");
  return true;
}

// Draw the initial Pip-Boy interface
void drawPipBoyInterface() {
  // Clear the display
  tft.fillScreen(PIP_BLACK);

  // Draw day of week at top
  tft.setTextSize(2);
  tft.setTextColor(PIP_GREEN);
  int dayWidth = dayOfWeek.length() * 12;
  tft.setCursor(screenCenterX - (dayWidth / 2), 25);
  tft.println(dayOfWeek);

  // Draw date
  tft.setTextSize(2);
  char dateStr[20];
  sprintf(dateStr, "%02d.%02d.%04d", day, month, year);
  int dateWidth = strlen(dateStr) * 12;
  tft.setCursor(screenCenterX - (dateWidth / 2), 50);
  tft.println(dateStr);

  // Try to load and display the GIF using the improved function
  bool gifLoaded = loadAndInitGIF("/vaultboy.gif");

  // If GIF loaded successfully, display the first frame
  if (gifLoaded) {
    gif.playFrame(true, NULL);
  } else {
    // If GIF loading failed, draw static figure
    // Head - simple circle with face
    tft.fillCircle(figureX, 100, 15, PIP_GREEN);

    // Eyes
    tft.fillCircle(figureX - 5, 97, 2, PIP_BLACK);
    tft.fillCircle(figureX + 5, 97, 2, PIP_BLACK);

    // Mouth - simple line for neutral/sad expression
    tft.drawFastHLine(figureX - 5, 105, 10, PIP_BLACK);

    // Body - simple rectangle
    tft.fillRect(figureX - 10, 115, 20, 30, PIP_GREEN);

    // Arms - simple lines
    tft.drawLine(figureX - 10, 120, figureX - 20, 130, PIP_GREEN);
    tft.drawLine(figureX + 10, 120, figureX + 20, 130, PIP_GREEN);

    // Legs - simple lines
    tft.drawLine(figureX - 5, 145, figureX - 5, 165, PIP_GREEN);
    tft.drawLine(figureX + 5, 145, figureX + 5, 165, PIP_GREEN);
  }

  // Draw time with larger font size
  tft.setTextSize(5);
  char timeStr[6];

  // Define time position
  int timeY = 80;

  // Hours
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d", displayHours);
  tft.setCursor(120, timeY);
  tft.println(timeStr);

  // Seconds (next to hours)
  tft.setTextSize(2);
  sprintf(timeStr, "%02d", seconds);
  tft.setCursor(195, timeY + 10);
  tft.println(timeStr);

  // Minutes
  tft.setTextSize(5);
  sprintf(timeStr, "%02d", minutes);
  tft.setCursor(120, timeY + 50);
  tft.println(timeStr);

  // AM/PM indicator (right of minutes)
  tft.setTextSize(2);
  tft.setCursor(195, timeY + 60);
  tft.println(hours >= 12 ? "PM" : "AM");

  // Draw PIP-BOY 3000 and ROBCO INDUSTRIES
  tft.setTextSize(2);

  // Calculate width to center text
  int pipBoyWidth = 11 * 12;  // "PIP-BOY 3000" is 11 chars
  int robcoWidth = 8 * 12;    // "ROBCO IND" is 8 chars

  tft.setCursor(screenCenterX - (pipBoyWidth / 2), 180);
  tft.println("PIP-BOY 3000");

  tft.setCursor(screenCenterX - (robcoWidth / 2), 200);
  tft.println("ROBCO IND");
}

// Function to update Pip-Boy time display
void updatePipBoyTime() {
  // Day of week at top
  tft.fillRect(screenCenterX - 70, 25, 140, 20, PIP_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(PIP_GREEN);
  int dayWidth = dayOfWeek.length() * 12;
  tft.setCursor(screenCenterX - (dayWidth / 2), 25);
  tft.println(dayOfWeek);

  // Date
  tft.fillRect(screenCenterX - 70, 50, 140, 20, PIP_BLACK);
  tft.setTextSize(2);
  char dateStr[20];
  sprintf(dateStr, "%02d.%02d.%04d", day, month, year);
  int dateWidth = strlen(dateStr) * 12;
  tft.setCursor(screenCenterX - (dateWidth / 2), 50);
  tft.println(dateStr);

  // Define time position
  int timeY = 80;
  char timeStr[6];

  // Hours
  tft.fillRect(120, timeY, 70, 40, PIP_BLACK);
  tft.setTextSize(5);
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d", displayHours);
  tft.setCursor(120, timeY);
  tft.println(timeStr);

  // Seconds (next to hours)
  tft.fillRect(195, timeY + 10, 30, 20, PIP_BLACK);
  tft.setTextSize(2);
  sprintf(timeStr, "%02d", seconds);
  tft.setCursor(195, timeY + 10);
  tft.println(timeStr);

  // Minutes
  tft.fillRect(120, timeY + 50, 70, 40, PIP_BLACK);
  tft.setTextSize(5);
  sprintf(timeStr, "%02d", minutes);
  tft.setCursor(120, timeY + 50);
  tft.println(timeStr);

  // AM/PM (next to minutes)
  tft.fillRect(195, timeY + 60, 30, 20, PIP_BLACK);
  tft.setTextSize(2);
  tft.setCursor(195, timeY + 60);
  tft.println(hours >= 12 ? "PM" : "AM");
}

// Function to update the GIF animation - fixed version
void updatePipBoyGif() {
  // Check if GIF exists and is loaded
  if (gifBuffer != NULL && gifSize > 0) {
    // Try to play the next frame
    if (!gif.playFrame(true, NULL)) {
      // End of animation, reset to beginning
      gif.reset();  // reset() returns void, not bool
      // The library will catch errors on next update
    }
  }
}

// Clean up resources when switching away from Pip-Boy mode
void cleanupPipBoyMode() {
  if (gifBuffer != NULL) {
    Serial.println("Cleaning up Pip-Boy GIF resources");
    gif.close();
    free(gifBuffer);
    gifBuffer = NULL;
    gifSize = 0;
    Serial.println("Pip-Boy GIF resources cleaned up");
  }
}

#endif  // PIPBOY_H
