/*
 * arc_digital.h - Arc Reactor Digital Clock Mode with JPEG Background
 */

#ifndef ARC_DIGITAL_H
#define ARC_DIGITAL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPIFFS.h>
#include <FS.h>
#include <TJpg_Decoder.h>
#include "utils.h"
#include "led_controls.h"

extern int CLOCK_VERTICAL_OFFSET;

// For Arc Reactor digital mode
int prevHours = -1, prevMinutes = -1, prevSeconds = -1;  // Track previous time values
bool prevColonState = false;                             // Track previous colon state
bool showColon = true;                                   // For blinking colon

// Define a constant for the background image to use
const char* DEFAULT_BACKGROUND = "/00_ironman.jpg";

#define CYAN_COLOR 0x07FF  // Cyan for text elements
#define TEXT_BACKGROUND_COLOR 0x0001  // Nearly black but not solid

// Function prototypes
void drawArcReactorBackground();
void updateDigitalTime();
void updateArcDigitalColon();
void resetArcDigitalVariables();
bool displayJPEGBackground(const char* filename);

// Callback function for the TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);
  return 1;  // Return 1 to decode next block
}

// Display JPEG background - simplified version
bool displayJPEGBackground(const char* filename) {
  // Check if file exists
  if (!SPIFFS.exists(filename)) {
    return false;
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

  // Try direct decoding method first (most efficient)
  bool decoded = TJpgDec.drawFsJpg(0, 0, filename);

  if (decoded) {
    return true;
  }

  // If direct method failed, try buffer method
  bool success = false;
  {
    File jpegFile = SPIFFS.open(filename, "r");
    if (!jpegFile) {
      return false;
    }

    // Get file size
    size_t fileSize = jpegFile.size();
    if (fileSize == 0) {
      jpegFile.close();
      return false;
    }

    // Use a smart pointer approach with std::unique_ptr
    uint8_t* jpegBuffer = nullptr;

    // Allocate buffer for the JPEG
    jpegBuffer = (uint8_t*)malloc(fileSize);
    if (!jpegBuffer) {
      jpegFile.close();
      return false;
    }

    // Read file into buffer
    size_t bytesRead = jpegFile.read(jpegBuffer, fileSize);
    jpegFile.close();

    if (bytesRead != fileSize) {
      free(jpegBuffer);
      return false;
    }

    // Try to decode from buffer
    success = TJpgDec.drawJpg(0, 0, jpegBuffer, fileSize);

    // Free buffer regardless of success/failure
    free(jpegBuffer);
  }

  return success;
}

void drawArcReactorBackground() {
  // Clear the display
  tft.fillScreen(TFT_BLACK);

  // Try to load JPEG background using the constant
  displayJPEGBackground(DEFAULT_BACKGROUND);
}

// Reset variables to force redraw of digital clock
void resetArcDigitalVariables() {
  prevHours = -1;
  prevMinutes = -1;
  prevSeconds = -1;
  prevColonState = false;
  showColon = true;
}

// Update digital time display - partial updates to improve performance
void updateDigitalTime() {
  // Only update parts that have changed
  bool hoursChanged = (hours != prevHours);
  bool minutesChanged = (minutes != prevMinutes);
  bool secondsChanged = (seconds != prevSeconds);
  bool colonChanged = (showColon != prevColonState);

  // Only update the display if the time or colon state has changed
  if (hoursChanged || minutesChanged || secondsChanged || colonChanged) {
    // Create backgrounds for text that preserve most of the underlying image
    tft.setTextColor(CYAN_COLOR, TEXT_BACKGROUND_COLOR);

    // Handle seconds update - at the top for symmetry
    if (seconds != prevSeconds) {
      // Format seconds with leading zero if needed
      char timeStr[6];
      sprintf(timeStr, seconds < 10 ? "0%d" : "%d", seconds);

      // Draw seconds with semi-transparent background
      tft.setTextSize(2);
      tft.setCursor(screenCenterX - 10, screenCenterY - 40 + CLOCK_VERTICAL_OFFSET);
      tft.print(timeStr);
    }

    // Handle hours update
    if (hours != prevHours || (minutes != prevMinutes && hours < 10)) {
      // Format hours with leading zero if needed
      char timeStr[6];
      int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
      sprintf(timeStr, displayHours < 10 ? "0%d" : "%d", displayHours);

      // Draw hours text with semi-transparent background
      tft.setTextSize(4);
      tft.setCursor(screenCenterX - 58, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
      tft.print(timeStr);
    }

    // Handle colon update (only if colon state changed)
    if (showColon != prevColonState) {
      // Colon position - apply vertical offset
      int colonX = screenCenterX - 15;
      int colonY = screenCenterY - 25 + CLOCK_VERTICAL_OFFSET;
      int colonWidth = 25;
      int colonHeight = 45;

      // Either draw the colon or clear its area by redrawing background
      if (showColon) {
        tft.setTextSize(4);
        tft.setCursor(screenCenterX - 10, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
        tft.print(":");
      } else {
        // When colon needs to be hidden, draw a small rect with the background color
        tft.fillRect(colonX, colonY, colonWidth, colonHeight, TEXT_BACKGROUND_COLOR);
      }
    }

    // Handle minutes update
    if (minutes != prevMinutes) {
      // Format minutes with leading zero if needed
      char timeStr[6];
      sprintf(timeStr, minutes < 10 ? "0%d" : "%d", minutes);

      // Draw minutes text with semi-transparent background - apply vertical offset
      tft.setTextSize(4);
      tft.setCursor(screenCenterX + 15, screenCenterY - 20 + CLOCK_VERTICAL_OFFSET);
      tft.print(timeStr);
    }

    // Handle AM/PM indicator (only in 12-hour mode and if hour changed)
    if (!is24Hour && (hours != prevHours || (prevHours == -1))) {
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
      tft.println(isPM ? "PM" : "AM");
    }

    // Save current state for next comparison
    prevHours = hours;
    prevMinutes = minutes;
    prevSeconds = seconds;
    prevColonState = showColon;
  }
}

// Specifically for the blinking colon in digital mode
void updateArcDigitalColon() {
  // Toggle colon state
  bool oldColonState = showColon;
  showColon = !showColon;

  // Only call update if colon state changed
  if (oldColonState != showColon) {
    // Position for colon - apply vertical offset
    int colonX = screenCenterX - 15;
    int colonY = screenCenterY - 25 + CLOCK_VERTICAL_OFFSET;
    int colonWidth = 25;
    int colonHeight = 45;

    if (showColon) {
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
    prevColonState = showColon;
  }
}

#endif  // ARC_DIGITAL_H
