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

// Ring settings - updated to make red ring visible near the edge of the screen
#define HOURS_RING_RADIUS 45      // Inner ring (hours) radius
#define MINUTES_RING_RADIUS 75    // Middle ring (minutes) radius
#define SECONDS_RING_RADIUS 105   // Outer ring (seconds) radius - near screen edge but fully visible
#define RING_THICKNESS 16         // Thickness of each ring

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
void forceCorrectRingDisplay();

// Initialize the Apple Rings theme - call when switching to this mode
void initAppleRingsTheme() {
  // Reset previous values to force a full redraw
  prevRingHours = -1;
  prevRingMinutes = -1;
  prevRingSeconds = -1;
  fullRedrawDone = false;
  
  Serial.println("Apple Rings theme initialized");
}

// Draw a ring segment with highly optimized smoothing techniques
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
  
  // Use much higher segment count for dramatically smoother appearance
  // Increase segments based on ring radius for better scaling
  int segments = max(120, (int)(radius * (endAngle - startAngle) / 40.0)); 
  float angleStep = (endRad - startRad) / segments;
  
  // Pre-calculate inner and outer radii with sub-pixel precision
  float innerRadiusF = radius - thickness/2.0;
  float outerRadiusF = radius + thickness/2.0;
  
  // Create temporary arrays to store points for a more consistent curve
  float* innerX = new float[segments+1];
  float* innerY = new float[segments+1];
  float* outerX = new float[segments+1];
  float* outerY = new float[segments+1];
  
  // Pre-compute all points with floating-point precision
  for (int i = 0; i <= segments; i++) {
    float angle = startRad + i * angleStep;
    
    innerX[i] = x + cos(angle) * innerRadiusF;
    innerY[i] = y + sin(angle) * innerRadiusF;
    outerX[i] = x + cos(angle) * outerRadiusF;
    outerY[i] = y + sin(angle) * outerRadiusF;
  }
  
  // Draw optimized triangles with consistent direction to reduce visual artifacts
  for (int i = 0; i < segments; i++) {
    // Use consistent winding order for triangles
    tft.fillTriangle(
      round(innerX[i]), round(innerY[i]),
      round(outerX[i]), round(outerY[i]),
      round(outerX[i+1]), round(outerY[i+1]),
      color
    );
    
    tft.fillTriangle(
      round(innerX[i]), round(innerY[i]),
      round(outerX[i+1]), round(outerY[i+1]),
      round(innerX[i+1]), round(innerY[i+1]),
      color
    );
  }
  
  // Clean up memory
  delete[] innerX;
  delete[] innerY;
  delete[] outerX;
  delete[] outerY;
  
  // Draw smoother end caps using multi-circle technique for better anti-aliasing effect
  if ((endAngle - startAngle) < 360) {
    // Calculate cap centers
    float startX_mid = x + cos(startRad) * radius;
    float startY_mid = y + sin(startRad) * radius;
    float endX_mid = x + cos(endRad) * radius;
    float endY_mid = y + sin(endRad) * radius;
    
    // Draw multi-layered caps for smoother appearance
    float capRadius = thickness/2.0;
    
    // Draw the main cap
    tft.fillCircle(round(startX_mid), round(startY_mid), round(capRadius), color);
    tft.fillCircle(round(endX_mid), round(endY_mid), round(capRadius), color);
    
    // Draw slightly smaller caps to fill potential gaps
    tft.fillCircle(round(startX_mid), round(startY_mid), round(capRadius*0.9), color);
    tft.fillCircle(round(endX_mid), round(endY_mid), round(capRadius*0.9), color);
  }
}

// Force correct display of all rings - improved initialization
void forceCorrectRingDisplay() {
  // Force all rings to update on first display
  prevRingHours = -1;
  prevRingMinutes = -1;
  prevRingSeconds = -1;
  
  // Clear the screen
  tft.fillScreen(APPLE_RINGS_BG);
  
  // Draw background rings only
  drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
           0, 360, APPLE_BLUE_BG);
  drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
           0, 360, APPLE_GREEN_BG);
  drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
           0, 360, APPLE_RED_BG);
  
  // Mark full redraw as done
  fullRedrawDone = true;
  
  // Each ring must be explicitly drawn based on current time
  
  // Hours ring
  float hourDegrees = is24Hour ? 15.0 : 30.0;
  int displayHours = is24Hour ? hours : (hours % 12);
  if (!is24Hour && displayHours == 0) displayHours = 12;
  
  if (displayHours > 0) {
    float endAngle = -90 + (displayHours * hourDegrees);
    drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
             -90, endAngle, APPLE_BLUE);
  }
  
  // Minutes ring
  if (minutes > 0) {
    float minuteAngle = -90 + (minutes * 6.0);
    drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
             -90, minuteAngle, APPLE_GREEN);
  }
  
  // Seconds ring
  if (seconds > 0) {
    float secondAngle = -90 + (seconds * 6.0);
    drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
             -90, secondAngle, APPLE_RED);
  }
  
  // Draw digital time
  drawTimeDigits();
  
  // Update previous values to match current time
  prevRingHours = hours;
  prevRingMinutes = minutes;
  prevRingSeconds = seconds;
}

// Draw the Apple Rings interface - full initialization
void drawAppleRingsInterface() {
  Serial.println("Drawing Apple Rings Interface");
  forceCorrectRingDisplay();
}

// Draw digital time in the center - improved spacing
void drawTimeDigits() {
  // Clear the center area completely
  tft.fillCircle(screenCenterX, screenCenterY, HOURS_RING_RADIUS - RING_THICKNESS/2, APPLE_RINGS_BG);
  
  // Display current time in digital format
  char timeStr[10];
  char secStr[3];
  char ampmStr[3];
  
  // Format time based on 12/24 hour setting
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  
  // Fix: Use wider spacing between hours and minutes
  sprintf(timeStr, "%02d:%02d", displayHours, minutes);
  sprintf(secStr, "%02d", seconds);
  sprintf(ampmStr, "%s", (hours >= 12 ? "PM" : "AM"));
  
  // Draw seconds on top with more space
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(screenCenterX - 6, screenCenterY - 22);
  tft.print(secStr);
  
  // Draw hours:minutes in center with better spacing
  tft.setTextSize(2);
  // Fix: Move text position to prevent overlap
  tft.setCursor(screenCenterX - 30, screenCenterY - 8);
  tft.print(timeStr);
  
  // Draw AM/PM at bottom with more space
  if (!is24Hour) {
    tft.setTextSize(1);
    tft.setCursor(screenCenterX - 6, screenCenterY + 16);
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
  
  // Update hours ring (innermost) - COMPLETELY FIXED
  if (hoursChanged) {
  // For 12-hour format: each hour = 30 degrees (360/12)
  // For 24-hour format: each hour = 15 degrees (360/24)
  float hourDegrees = is24Hour ? 15.0 : 30.0;
  
  // Convert current hour to appropriate format
  int displayHours = is24Hour ? hours : (hours % 12);
  
  // IMPORTANT: In 12-hour mode, 0 is 12
  if (!is24Hour && displayHours == 0) {
    displayHours = 12;
  }
  
  // Midnight transition - full reset to transparent
  if (hours == 0 && (prevRingHours == 23 || prevRingHours == 11)) {
    // At midnight, clear everything to background color first
    drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS + 2, 
             0, 360, APPLE_RINGS_BG);
             
    // Then draw ONLY the background - keep foreground transparent
    drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
             0, 360, APPLE_BLUE_BG);
             
    // DO NOT draw any foreground at hour 0 (midnight)
  }
  // Normal hour transitions
  else {
    // Clear with background color for clean redraw if needed
    if (prevRingHours == -1) {
      drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS + 2, 
               0, 360, APPLE_RINGS_BG);
    }
    
    // Draw background ring
    drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
             0, 360, APPLE_BLUE_BG);
    
    // Calculate end angle based on hours (start from 12 o'clock position)
    float endAngle = -90 + (displayHours * hourDegrees);
    
    // Draw the foreground ring based on hour value
    if (displayHours > 0) {
      // Normal hour value - draw progress
      drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
               -90, endAngle, APPLE_BLUE);
    } else if (!is24Hour && displayHours == 12) {
      // Noon in 12-hour mode - draw full circle
      drawRing(screenCenterX, screenCenterY, HOURS_RING_RADIUS, RING_THICKNESS, 
               -90, 270, APPLE_BLUE);
    }
  }
  
  prevRingHours = hours;
}
  
  // Update minutes ring (middle) - IMPROVED FIX FOR ARTIFACTS
  if (minutesChanged) {
    // Each minute = 6 degrees (360/60)
    float minuteDegrees = 6.0;
    
    // Check if we need to completely reset the ring
    // This happens at minute rollover or first initialization
    bool minuteReset = (minutes == 0 && prevRingMinutes > 0) || prevRingMinutes == -1;
    
    if (minuteReset) {
      // First clear with background color (slightly wider)
      drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS + 2, 
               0, 360, APPLE_RINGS_BG);
               
      // Then redraw the proper background
      drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_GREEN_BG);
               
      // Don't draw any foreground for minute 0 - this prevents artifacts
      // The minute hand will start drawing again at minute 1
    } else if (minutes < prevRingMinutes) {
      // Handle unusual case (time adjustment)
      // Clear and redraw background
      drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_GREEN_BG);
               
      // Draw proper segment for current minutes
      if (minutes > 0) {
        float endAngle = -90 + (minutes * minuteDegrees);
        drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
                 -90, endAngle, APPLE_GREEN);
      }
    } else {
      // Normal minute progression
      // Calculate end angle based on minutes
      float endAngle = -90 + (minutes * minuteDegrees);
      
      // Draw the proper segment for current minutes
      drawRing(screenCenterX, screenCenterY, MINUTES_RING_RADIUS, RING_THICKNESS, 
               -90, endAngle, APPLE_GREEN);
    }
    
    prevRingMinutes = minutes;
  }
  
  // Update seconds ring (outermost) - ENHANCED FIX FOR ARTIFACTS AND FLICKERING
  if (secondsChanged) {
    // Each second = 6 degrees (360/60)
    float secondDegrees = 6.0;
    
    // Complete reset case - seconds rollover from 59 to 0
    if (seconds == 0 && prevRingSeconds > 0) {
      // Clear the entire screen area for the seconds ring with the background color
      // Use a slightly larger width to ensure all artifacts are removed
      drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS + 2, 
               0, 360, APPLE_RINGS_BG);
               
      // Then redraw the proper background
      drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_RED_BG);
               
      // Don't draw any foreground for second 0 - this prevents artifacts
      // The second hand will start drawing again at second 1
      
    } else if (prevRingSeconds == -1) {
      // First draw case (initialization)
      // Draw the background ring
      drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
               0, 360, APPLE_RED_BG);
               
      // Draw the initial arc
      if (seconds > 0) {
        float endAngle = -90 + (seconds * secondDegrees);
        drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
                 -90, endAngle, APPLE_RED);
      }
    } else {
      // Normal update for seconds 1-59
      // Calculate end angle based on seconds (start from 12 o'clock position)
      float endAngle = -90 + (seconds * secondDegrees);
      
      // If seconds decreased (unusual case like time adjustment), redraw background
      if (seconds < prevRingSeconds) {
        drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
                 0, 360, APPLE_RED_BG);
      }
      
      // Draw just the new segment(s)
      if (seconds > 0) {
        drawRing(screenCenterX, screenCenterY, SECONDS_RING_RADIUS, RING_THICKNESS, 
                 -90, endAngle, APPLE_RED);
      }
    }
    
    // Save the current seconds value
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
