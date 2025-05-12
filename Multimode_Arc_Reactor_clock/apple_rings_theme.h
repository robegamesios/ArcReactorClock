/*
 * apple_rings_theme.h - Apple Activity Rings-inspired Clock Mode
 * For Multi-Mode Digital Clock project
 * Features colored concentric rings for hours, minutes, and seconds
 */

#ifndef APPLE_RINGS_THEME_H
#define APPLE_RINGS_THEME_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "utils.h"
#include "led_controls.h"

// Define a constant for the Apple Rings mode
#define MODE_APPLE_RINGS 5  // Add a new mode for Apple Rings
#define APPLE_RINGS_BG TFT_BLACK  // Black background

// Define colors for rings (close to Apple Activity rings)
#define APPLE_BLUE 0x04FF   // Blue ring for hours (inner)
#define APPLE_GREEN 0x07E0  // Green ring for minutes (middle)
#define APPLE_RED 0xF800    // Red ring for seconds (outer)

// Define colors for background rings
#define APPLE_BLUE_BG 0x0219   // Dark blue for hours background
#define APPLE_GREEN_BG 0x0300  // Dark green for minutes background
#define APPLE_RED_BG 0x4000    // Dark red for seconds background

// Ring settings - increased to use more screen space
#define HOURS_RING_RADIUS 45     // Inner ring (hours) radius
#define MINUTES_RING_RADIUS 75   // Middle ring (minutes) radius
#define SECONDS_RING_RADIUS 105  // Outer ring (seconds) radius
#define RING_THICKNESS 16        // Thickness of each ring

// Track previous time values for optimized updating
int prevRingHours = -1;
int prevRingMinutes = -1;
int prevRingSeconds = -1;

// Flag to avoid excessive redraws and reduce flickering
bool fullRedrawDone = false;

// Function prototypes
void initAppleRingsTheme();
void drawAppleRingsInterface();
void updateAppleRingsTime();
void drawRing(int x, int y, int radius, int thickness, float startAngle, float endAngle, uint16_t color);
void drawTimeDigits();
void updateTimeDigits();
void cleanupAppleRingsMode();

// Initialize the Apple Rings theme - call when switching to this mode
void initAppleRingsTheme() {
  // Reset previous values to force a full redraw
  prevRingHours = -1;
  prevRingMinutes = -1;
  prevRingSeconds = -1;
  fullRedrawDone = false;
  
  Serial.println("Apple Rings theme initialized");
}

// Draw a ring segment with the specified parameters - improved for cleaner edges
void drawRing(int x, int y, int radius, int thickness, float startAngle, float endAngle, uint16_t color) {
  // Handle case where endAngle < startAngle (crossing 0/360 boundary)
  if (endAngle < startAngle) {
    drawRing(x, y, radius, thickness, startAngle, 360, color);
    drawRing(x, y, radius, thickness, 0, endAngle, color);
    return;
  }
  
  // Convert angles from degrees to radians
  float startRad = startAngle * DEG_TO_RAD;
  float endRad = endAngle * DEG_TO_RAD;
  
  // Draw filled arc by segments for smoother appearance
  // Use more segments for longer arcs
  int segments = max(20, (int)((endAngle - startAngle) / 5));
  float angleStep = (endRad - startRad) / segments;
  
  // Pre-calculate inner and outer radii
  int innerRadius = radius - thickness/2;
  int outerRadius = radius + thickness/2;
  
  // For small arcs, simplify with a single shape
  if (endAngle - startAngle <= 5) {
    float midRad = (startRad + endRad) / 2;
    int x1 = x + round(cos(startRad) * innerRadius);
    int y1 = y + round(sin(startRad) * innerRadius);
    int x2 = x + round(cos(startRad) * outerRadius);
    int y2 = y + round(sin(startRad) * outerRadius);
    int x3 = x + round(cos(endRad) * outerRadius);
    int y3 = y + round(sin(endRad) * outerRadius);
    int x4 = x + round(cos(endRad) * innerRadius);
    int y4 = y + round(sin(endRad) * innerRadius);
    
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    tft.fillTriangle(x1, y1, x3, y3, x4, y4, color);
    return;
  }
  
  // Draw the arc as a series of triangles
  for (int i = 0; i < segments; i++) {
    float angle1 = startRad + i * angleStep;
    float angle2 = startRad + (i + 1) * angleStep;
    
    int x1 = x + round(cos(angle1) * innerRadius);
    int y1 = y + round(sin(angle1) * innerRadius);
    int x2 = x + round(cos(angle1) * outerRadius);
    int y2 = y + round(sin(angle1) * outerRadius);
    int x3 = x + round(cos(angle2) * outerRadius);
    int y3 = y + round(sin(angle2) * outerRadius);
    int x4 = x + round(cos(angle2) * innerRadius);
    int y4 = y + round(sin(angle2) * innerRadius);
    
    tft.fillTriangle(x1, y1, x2, y2, x3, y3, color);
    tft.fillTriangle(x1, y1, x3, y3, x4, y4, color);
  }
}

// Draw the Apple Rings interface - full initialization
void drawAppleRingsInterface() {
  Serial.println("Drawing Apple Rings Interface");
  
  // Clear the screen
  tft.fillScreen(APPLE_RINGS_BG);
  
  // Draw complete background rings
  // Hours ring (inner) - blue
  drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
           0, 360, APPLE_BLUE_BG);
  
  // Minutes ring (middle) - green
  drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
           0, 360, APPLE_GREEN_BG);
  
  // Seconds ring (outer) - red
  drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
           0, 360, APPLE_RED_BG);
  
  // Mark full redraw as done
  fullRedrawDone = true;
  
  // Initialize rings with current time
  updateAppleRingsTime();
}

// Draw digital time in the center - smaller and more compact
void drawTimeDigits() {
  // Clear the center area
  tft.fillCircle(screenCenterX, screenCenterY, HOURS_RING_RADIUS - RING_THICKNESS - 2, APPLE_RINGS_BG);
  
  // Display current time in digital format
  char timeStr[10];
  char secStr[3];
  char ampmStr[3];
  
  // Format time based on 12/24 hour setting
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d:%02d", displayHours, minutes);
  sprintf(secStr, "%02d", seconds);
  sprintf(ampmStr, "%s", (hours >= 12 ? "PM" : "AM"));
  
  // Draw seconds on top
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(screenCenterX - 6, screenCenterY - 18);
  tft.print(secStr);
  
  // Draw hours:minutes in center - smaller size with better spacing
  tft.setTextSize(2);
  tft.setCursor(screenCenterX - 28, screenCenterY - 8);
  tft.print(timeStr);
  
  // Draw AM/PM at bottom
  if (!is24Hour) {
    tft.setTextSize(1);
    tft.setCursor(screenCenterX - 6, screenCenterY + 12);
    tft.print(ampmStr);
  }
}

// Update only the digital time without redrawing rings
void updateTimeDigits() {
  drawTimeDigits();
}

// Update all rings based on current time - optimized to reduce flickering
void updateAppleRingsTime() {
  // Make sure background is drawn first if needed
  if (!fullRedrawDone) {
    drawAppleRingsInterface();
    return;
  }
  
  // Check if time components have changed
  bool hoursChanged = (hours != prevRingHours);
  bool minutesChanged = (minutes != prevRingMinutes);
  bool secondsChanged = (seconds != prevRingSeconds);
  
  // Update hours ring (innermost)
  if (hoursChanged) {
    // For 12-hour format: each hour = 30 degrees (360/12)
    // For 24-hour format: each hour = 15 degrees (360/24)
    float hourDegrees = is24Hour ? 15.0 : 30.0;
    
    // Convert current hour to appropriate format
    int displayHours = is24Hour ? hours : (hours % 12 == 0 ? 12 : hours % 12);
    
    // Only redraw background if hours changed from 12/24 to 1
    if (prevRingHours == 12 || prevRingHours == 24 || prevRingHours == -1) {
      drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_BLUE_BG);
    }
    
    // Calculate end angle based on hours (start from 12 o'clock position)
    float endAngle = -90 + (displayHours * hourDegrees);
    if (displayHours == 0 || displayHours == 12 || displayHours == 24) {
      endAngle = 270; // Full circle for 12 or 24 hours
    }
    
    // Draw the hour progress ring
    drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
             -90, endAngle, APPLE_BLUE);
    
    prevRingHours = hours;
  }
  
  // Update minutes ring (middle)
  if (minutesChanged) {
    // Each minute = 6 degrees (360/60)
    float minuteDegrees = 6.0;
    
    // Only redraw background if minutes changed from 59 to 0
    if (prevRingMinutes == 59 || prevRingMinutes == -1) {
      drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_GREEN_BG);
    }
    
    // Calculate end angle based on minutes (start from 12 o'clock position)
    float endAngle = -90 + (minutes * minuteDegrees);
    if (minutes == 0) {
      endAngle = 270; // Full circle for 60 minutes
    }
    
    // Draw the minute progress ring
    drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
             -90, endAngle, APPLE_GREEN);
    
    prevRingMinutes = minutes;
  }
  
  // Update seconds ring (outermost)
  if (secondsChanged) {
    // Each second = 6 degrees (360/60)
    float secondDegrees = 6.0;
    
    // Only redraw background if seconds changed from 59 to 0
    if (prevRingSeconds == 59 || prevRingSeconds == -1) {
      drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_RED_BG);
    }
    
    // Calculate end angle based on seconds (start from 12 o'clock position)
    float endAngle = -90 + (seconds * secondDegrees);
    if (seconds == 0) {
      endAngle = 270; // Full circle for 60 seconds
    }
    
    // Draw the second progress ring
    drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
             -90, endAngle, APPLE_RED);
    
    prevRingSeconds = seconds;
  }
  
  // Update the digital time display if any time component changed
  if (hoursChanged || minutesChanged || secondsChanged) {
    updateTimeDigits();
  }
}

// Cleanup function - called when switching away from this mode
void cleanupAppleRingsMode() {
  fullRedrawDone = false;
}

#endif // APPLE_RINGS_THEME_H
