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
#include <FS.h> // Include FS for File class

// Function prototypes
void drawPipBoyInterface();
void updatePipBoyTime();
void GIFDraw(GIFDRAW *pDraw); // Forward declaration

// Global variables
AnimatedGIF gif;
const int figureX = 75; // Defined here to fix scope issue
uint8_t *gifBuffer = NULL; // Buffer to hold GIF data
int gifSize = 0;

//////////////////////////////////////////////
// PIP-BOY MODE FUNCTIONS
//////////////////////////////////////////////

void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *d, *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth > tft.width())
    iWidth = tft.width();

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y;  // current line

  s = pDraw->pPixels;
  if (pDraw->ucDisposalMethod == 2) {  // restore to background color
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
        usTemp[x] = PIP_BLACK;
      else
        usTemp[x] = usPalette[s[x]];
    }
    d = usTemp;
  } else {
    for (x = 0; x < iWidth; x++) {
      if (s[x] == pDraw->ucTransparent)
        continue;
      usTemp[x] = usPalette[s[x]];
    }
    d = usTemp;
  }

  tft.pushImage(pDraw->iX, y, iWidth, 1, d);
}

void drawPipBoyInterface() {
  // Clear the display
  tft.fillScreen(PIP_BLACK);

  // Draw day of week at top - Using appropriate font size
  tft.setTextSize(2);
  tft.setTextColor(PIP_GREEN);
  int dayWidth = dayOfWeek.length() * 12;             // Approximate width for font
  tft.setCursor(screenCenterX - (dayWidth / 2), 25);  // Moved lower from 10 to 25
  tft.println(dayOfWeek);

  // Draw date with larger font as requested
  tft.setTextSize(2);  // Increased from 1 to 2
  char dateStr[20];
  sprintf(dateStr, "%02d.%02d.%04d", day, month, year);
  // Calculate width to center text
  int dateWidth = strlen(dateStr) * 12;                // Approximate width for font size 2
  tft.setCursor(screenCenterX - (dateWidth / 2), 50);  // Position below day of week
  tft.println(dateStr);

  // Try to load and display the GIF
  if (SPIFFS.exists("/vaultboy.gif")) {
    fs::File f = SPIFFS.open("/vaultboy.gif", "r");
    if (f) {
      // Get the size of the file
      gifSize = f.size();
      
      // Allocate a buffer to hold the GIF data
      if (gifBuffer != NULL) {
        free(gifBuffer);
      }
      gifBuffer = (uint8_t *)malloc(gifSize);
      
      if (gifBuffer != NULL) {
        // Read the file into the buffer
        f.read(gifBuffer, gifSize);
        f.close();
        
        // Initialize and open the GIF
        gif.begin(GIF_PALETTE_RGB565_LE);
        if (gif.open(gifBuffer, gifSize, GIFDraw)) {
          // Successfully opened the GIF, play one frame
          gif.playFrame(true, NULL);
          Serial.println("GIF opened successfully");
        } else {
          Serial.println("Failed to open GIF");
          free(gifBuffer);
          gifBuffer = NULL;
          gifSize = 0;
        }
      } else {
        Serial.println("Failed to allocate memory for GIF");
        f.close();
      }
    }
  }
  
  // If GIF loading failed, draw static figure
  if (gifBuffer == NULL) {
    // Add a static Pip-Boy figure - moved slightly to the right from the far left
    // Just simple face and body
    // figureX is now globally defined

    // Head - simple circle with face
    tft.fillCircle(figureX, 100, 15, PIP_GREEN);  // Made slightly larger

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
  tft.setTextSize(5);  // Increased size
  char timeStr[6];

  // Lower the time display slightly
  int timeY = 80;  // Starting Y position for hours

  // Hours
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d", displayHours);
  tft.setCursor(120, timeY);  // Left-aligned time
  tft.println(timeStr);

  // Seconds (next to hours)
  tft.setTextSize(2);
  sprintf(timeStr, "%02d", seconds);
  tft.setCursor(195, timeY + 10);  // Right of hours
  tft.println(timeStr);

  // Minutes
  tft.setTextSize(5);
  sprintf(timeStr, "%02d", minutes);
  tft.setCursor(120, timeY + 50);  // Below hours
  tft.println(timeStr);

  // AM/PM indicator (right of minutes)
  tft.setTextSize(2);
  tft.setCursor(195, timeY + 60);  // Right of minutes
  tft.println(hours >= 12 ? "PM" : "AM");

  // Draw PIP-BOY 3000 and ROBCO INDUSTRIES higher up on the screen
  tft.setTextSize(2);

  // Calculate width to center text
  int pipBoyWidth = 11 * 12;  // "PIP-BOY 3000" is 11 chars, approx 12 pixels per char at size 2
  int robcoWidth = 8 * 12;    // "ROBCO IND" is 8 chars

  // Position them higher on the screen
  tft.setCursor(screenCenterX - (pipBoyWidth / 2), 180);
  tft.println("PIP-BOY 3000");

  tft.setCursor(screenCenterX - (robcoWidth / 2), 200);
  tft.println("ROBCO IND");
}

// Function to update the GIF animation (call this in your main loop)
void updatePipBoyGif() {
  // Check if GIF exists and is loaded
  if (gifBuffer != NULL && gifSize > 0) {
    // Try to play the next frame
    if (!gif.playFrame(true, NULL)) {
      // End of animation, reset to beginning
      gif.reset();
    }
  }
}

// Function to update Pip-Boy time display
void updatePipBoyTime() {
  // Day of week at top
  tft.fillRect(screenCenterX - 70, 25, 140, 20, PIP_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(PIP_GREEN);
  int dayWidth = dayOfWeek.length() * 12;  // Approximate width for font size 2
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

  // Hours - with larger font
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

  // Minutes - with larger font
  tft.fillRect(120, timeY + 50, 70, 40, PIP_BLACK);
  tft.setTextSize(5);
  sprintf(timeStr, "%02d", minutes);
  tft.setCursor(120, timeY + 50);
  tft.println(timeStr);

  // AM/PM (next to minutes)
  tft.fillRect(195, timeY + 60, 30, 20, PIP_BLACK);
  tft.setTextSize(2);
  tft.setCursor(195, timeY + 60);
  tft.println((is24Hour || hours < 12) ? "AM" : "PM");
}

// Clean up resources when switching away from Pip-Boy mode
void cleanupPipBoyMode() {
  if (gifBuffer != NULL) {
    gif.close();
    free(gifBuffer);
    gifBuffer = NULL;
    gifSize = 0;
  }
}

#endif  // PIPBOY_H
