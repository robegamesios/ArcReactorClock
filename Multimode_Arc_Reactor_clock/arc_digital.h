/*
 * arc_digital.h - Arc Reactor Digital Clock Mode
 * For Multi-Mode Digital Clock project
 * Updated with Iron Man color scheme
 */

#ifndef ARC_DIGITAL_H
#define ARC_DIGITAL_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "utils.h"

// For Arc Reactor digital mode
int prevHours = -1, prevMinutes = -1, prevSeconds = -1; // Track previous time values
bool prevColonState = false;                           // Track previous colon state
bool showColon = true;                                 // For blinking colon
uint16_t bgColor = 0x000A;                             // Very dark blue background color

// Iron Man color scheme
#define IRONMAN_RED 0xF800    // Bright red for the solid ring
#define IRONMAN_GOLD 0xFD20   // Gold color for the outer circle
#define IRONMAN_CYAN 0x07FF   // Keep the cyan for the text elements

// Function prototypes
void drawArcReactorBackground();
void updateDigitalTime();
void updateArcDigitalColon();
void resetArcDigitalVariables();

// Draw Arc Reactor background for both digital and analog modes
void drawArcReactorBackground() {
  // Clear the display
  tft.fillScreen(TFT_BLACK);
  
  // Draw outer circle of Arc Reactor - now gold
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius, IRONMAN_GOLD);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius - 1, IRONMAN_GOLD);
  
  // Draw inner circle of Arc Reactor with red background (formerly navy)
  tft.fillCircle(screenCenterX, screenCenterY, screenRadius * 0.85, IRONMAN_RED);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.85, IRONMAN_CYAN);
    
  // Draw inner glowing center of Arc Reactor (keep dark blue for contrast)
  tft.fillCircle(screenCenterX, screenCenterY, screenRadius * 0.65, bgColor); // Very dark blue
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.65, IRONMAN_CYAN);
  
  // Draw decorative circles for Arc Reactor effect
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.55, IRONMAN_GOLD);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.45, IRONMAN_GOLD);
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
  // Only update the display if the time or colon state has changed
  if (hours != prevHours || minutes != prevMinutes || seconds != prevSeconds || showColon != prevColonState) {
    // Handle seconds update - at the top for symmetry
    if (seconds != prevSeconds) {
      // Format seconds with leading zero if needed 
      char timeStr[6];
      if (seconds < 10) {
        sprintf(timeStr, "0%d", seconds);
      } else {
        sprintf(timeStr, "%d", seconds);
      }
      
      // Clear and redraw seconds area at the top of the display
      tft.fillRect(screenCenterX - 15, screenCenterY - 50, 40, 20, bgColor);
      tft.setTextSize(2);
      tft.setTextColor(IRONMAN_CYAN);
      tft.setCursor(screenCenterX - 10, screenCenterY - 50);
      tft.print(timeStr);
    }

    // Handle hours update
    if (hours != prevHours || (minutes != prevMinutes && hours < 10)) {
      // Format hours with leading zero if needed
      char timeStr[6];
      int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
      if (displayHours < 10) {
        sprintf(timeStr, "0%d", displayHours);
      } else {
        sprintf(timeStr, "%d", displayHours);
      }
      
      // Clear and redraw hours area only if it changed
      tft.fillRect(screenCenterX - 63, screenCenterY - 25, 65, 45, bgColor);
      tft.setTextSize(4);
      tft.setTextColor(IRONMAN_CYAN);
      tft.setCursor(screenCenterX - 58, screenCenterY - 20);
      tft.print(timeStr);
    }
    
    // Handle colon update (only if colon state changed)
    if (showColon != prevColonState) {
      tft.fillRect(screenCenterX - 15, screenCenterY - 25, 25, 45, bgColor);
      if (showColon) {
        tft.setTextSize(4);
        tft.setTextColor(IRONMAN_CYAN);
        tft.setCursor(screenCenterX - 10, screenCenterY - 20);
        tft.print(":");
      }
    }
    
    // Handle minutes update
    if (minutes != prevMinutes) {
      // Format minutes with leading zero if needed
      char timeStr[6];
      if (minutes < 10) {
        sprintf(timeStr, "0%d", minutes);
      } else {
        sprintf(timeStr, "%d", minutes);
      }
      
      // Clear and redraw minutes area only if it changed
      tft.fillRect(screenCenterX + 10, screenCenterY - 25, 50, 45, bgColor);
      tft.setTextSize(4);
      tft.setTextColor(IRONMAN_CYAN);
      tft.setCursor(screenCenterX + 10, screenCenterY - 20);
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
      
      // Position for AM/PM indicator
      tft.fillRect(screenCenterX - 15, screenCenterY + 35, 40, 20, bgColor);
      tft.setTextSize(2);
      tft.setTextColor(IRONMAN_CYAN);
      if (isPM) {
        tft.setCursor(screenCenterX - 10, screenCenterY + 35);
        tft.println("PM");
      } else {
        tft.setCursor(screenCenterX - 10, screenCenterY + 35);
        tft.println("AM");
      }
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
    // Update only the colon part, not the entire time display
    tft.fillRect(screenCenterX - 15, screenCenterY - 25, 25, 45, bgColor);
    if (showColon) {
      tft.setTextSize(4);
      tft.setTextColor(IRONMAN_CYAN);
      tft.setCursor(screenCenterX - 10, screenCenterY - 20);
      tft.print(":");
    }
    
    // Update the state tracking
    prevColonState = showColon;
  }
}

#endif // ARC_DIGITAL_H
