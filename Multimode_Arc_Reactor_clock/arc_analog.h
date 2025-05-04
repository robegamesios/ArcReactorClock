/*
 * arc_analog.h - Arc Reactor Analog Clock Mode
 * For Multi-Mode Digital Clock project
 * Features hour/minute hands and solid outer progress ring for seconds
 */

#ifndef ARC_ANALOG_H
#define ARC_ANALOG_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "utils.h"

extern bool needClockRefresh;

// Track previous hand positions for clean updates
int prevMinuteX = -1, prevMinuteY = -1;
int prevHourX = -1, prevHourY = -1;
int prevSecond = -1;

// Store last angle positions
float prevMinuteAngle = -1, prevHourAngle = -1;

// Define colors for better visibility
#define HOUR_HAND_COLOR TFT_WHITE
#define MINUTE_HAND_COLOR 0xFFE0  // Bright yellow
#define SECOND_RING_COLOR 0xF800  // Bright red
#define CENTER_DOT_COLOR 0x07FF   // Cyan
#define HOUR_MARKER_COLOR 0x07FF  // Cyan for hour markers

// Ring settings
#define RING_THICKNESS 4  // Thickness of the seconds ring in pixels
#define RING_RADIUS 120   // Absolute radius for a 240x240 screen (slightly beyond visible circle)

// Function prototypes
void drawAnalogClock();
void updateAnalogClock();
void initAnalogClock();
void drawClockFace();
void drawSecondsArc(int x, int y, int start_angle, int end_angle, int r, int thickness, unsigned int color);

// Initialize the analog clock variables
void initAnalogClock() {
  // Reset previous positions to force clean redraw
  prevMinuteX = prevMinuteY = -1;
  prevHourX = prevHourY = -1;
  prevSecond = -1;
  prevMinuteAngle = prevHourAngle = -1;

  // Draw the clock face (hour markers)
  drawClockFace();
}

// Draw the static clock face elements (hour markers)
void drawClockFace() {
  // Draw hour markers (12 dots around the clock)
  for (int i = 0; i < 12; i++) {
    float angle = i * 30 * DEG_TO_RAD;                           // 30 degrees per hour mark
    int x = screenCenterX + sin(angle) * (screenRadius * 0.95);  // Use 0.95 to push closer to edge
    int y = screenCenterY - cos(angle) * (screenRadius * 0.95);

    tft.fillCircle(x, y, 3, HOUR_MARKER_COLOR);
  }
}

// Optimized function to draw arc segments for seconds - reduces flicker
void drawSecondsArc(int x, int y, int start_angle, int end_angle, int r, int thickness, unsigned int color) {
  // Calculate the actual angles in radians
  float start_rad = start_angle * DEG_TO_RAD;
  float end_rad = end_angle * DEG_TO_RAD;

  // Instead of drawing the entire arc as segments, just draw the end points
  // and connect them with lines - this is much faster and reduces flicker

  // Calculate the 4 corner points of the arc segment
  int x0 = x + cos(start_rad) * (r - thickness);
  int y0 = y + sin(start_rad) * (r - thickness);
  int x1 = x + cos(start_rad) * r;
  int y1 = y + sin(start_rad) * r;
  int x2 = x + cos(end_rad) * r;
  int y2 = y + sin(end_rad) * r;
  int x3 = x + cos(end_rad) * (r - thickness);
  int y3 = y + sin(end_rad) * (r - thickness);

  // Draw the filled segment as two triangles
  tft.fillTriangle(x0, y0, x1, y1, x2, y2, color);
  tft.fillTriangle(x0, y0, x2, y2, x3, y3, color);
}

// Draw full analog clock (background is drawn separately)
void drawAnalogClock() {
  // Draw clock face (hour markers)
  drawClockFace();

  // Calculate current hand angles
  float minuteAngle = minutes * 6 + (seconds * 0.1);  // 6 degrees per minute + slight adjustment for seconds
  float hourAngle = hours * 30 + (minutes * 0.5);     // 30 degrees per hour + adjustment for minutes

  // Get the hand lengths based on clock radius
  int minuteHandLength = screenRadius * 0.7;
  int hourHandLength = screenRadius * 0.5;

  // Draw hour hand
  float hourRad = hourAngle * DEG_TO_RAD;
  int hourX = screenCenterX + sin(hourRad) * hourHandLength;
  int hourY = screenCenterY - cos(hourRad) * hourHandLength;
  tft.drawLine(screenCenterX, screenCenterY, hourX, hourY, HOUR_HAND_COLOR);

  // Save current hour hand position
  prevHourX = hourX;
  prevHourY = hourY;
  prevHourAngle = hourAngle;

  // Draw minute hand
  float minuteRad = minuteAngle * DEG_TO_RAD;
  int minuteX = screenCenterX + sin(minuteRad) * minuteHandLength;
  int minuteY = screenCenterY - cos(minuteRad) * minuteHandLength;
  tft.drawLine(screenCenterX, screenCenterY, minuteX, minuteY, MINUTE_HAND_COLOR);

  // Save current minute hand position
  prevMinuteX = minuteX;
  prevMinuteY = minuteY;
  prevMinuteAngle = minuteAngle;

  // For initial draw, draw the seconds ring in segments to avoid flicker
  // Draw from 270 degrees (top/12 o'clock) to current second position
  for (int i = 0; i < seconds; i++) {
    int startAngle = 270 + (i * 6);
    int endAngle = startAngle + 6;
    drawSecondsArc(screenCenterX, screenCenterY, startAngle, endAngle, RING_RADIUS, RING_THICKNESS, SECOND_RING_COLOR);
  }

  prevSecond = seconds;

  // Draw center dot
  tft.fillCircle(screenCenterX, screenCenterY, 5, CENTER_DOT_COLOR);
}

// Update analog clock - using incremental updates to reduce flicker
void updateAnalogClock() {
  // Calculate current hand angles
  float minuteAngle = minutes * 6 + (seconds * 0.1);  // 6 degrees per minute + adjustment for seconds
  float hourAngle = hours * 30 + (minutes * 0.5);     // 30 degrees per hour + adjustment for minutes

  // Get the hand lengths based on clock radius
  int minuteHandLength = screenRadius * 0.7;
  int hourHandLength = screenRadius * 0.5;

  // Check for periodic full refresh (every 15 minutes)
  if (seconds == 0 && minutes % 5 == 0) {
    // Signal for a full refresh - we'll use a global variable
    needClockRefresh = true;
    return;
  }

  // Update second ring if it changed
  if (seconds != prevSecond) {
    // Check for 15-minute mark to trigger full refresh
    if (seconds == 0 && minutes % 15 == 0) {
      needClockRefresh = true;
      prevSecond = seconds;
      return;  // Skip the rest of the update
    }
    if (seconds == 0) {

      // First, clear the seconds ring
      for (int i = 0; i < 60; i++) {
        int startAngle = 270 + (i * 6);
        int endAngle = startAngle + 6;
        drawSecondsArc(screenCenterX, screenCenterY, startAngle, endAngle, RING_RADIUS, RING_THICKNESS, TFT_BLACK);
      }

      // Clean the minute hand with thicker lines
      if (prevMinuteX != -1 && prevMinuteY != -1) {
        for (int i = -2; i <= 2; i++) {
          for (int j = -2; j <= 2; j++) {
            tft.drawLine(screenCenterX + i, screenCenterY + j,
                         prevMinuteX + i, prevMinuteY + j, TFT_BLACK);
          }
        }
      }

      // Clean the hour hand with thicker lines
      if (prevHourX != -1 && prevHourY != -1) {
        for (int i = -2; i <= 2; i++) {
          for (int j = -2; j <= 2; j++) {
            tft.drawLine(screenCenterX + i, screenCenterY + j,
                         prevHourX + i, prevHourY + j, TFT_BLACK);
          }
        }
      }

      // Redraw the clock face elements that might have been affected
      drawClockFace();

      // Calculate new positions for hands
      float hourRad = hourAngle * DEG_TO_RAD;
      int hourX = screenCenterX + sin(hourRad) * hourHandLength;
      int hourY = screenCenterY - cos(hourRad) * hourHandLength;

      float minuteRad = minuteAngle * DEG_TO_RAD;
      int minuteX = screenCenterX + sin(minuteRad) * minuteHandLength;
      int minuteY = screenCenterY - cos(minuteRad) * minuteHandLength;

      // Draw hour hand
      tft.drawLine(screenCenterX, screenCenterY, hourX, hourY, HOUR_HAND_COLOR);
      prevHourX = hourX;
      prevHourY = hourY;
      prevHourAngle = hourAngle;

      // Draw minute hand
      tft.drawLine(screenCenterX, screenCenterY, minuteX, minuteY, MINUTE_HAND_COLOR);
      prevMinuteX = minuteX;
      prevMinuteY = minuteY;
      prevMinuteAngle = minuteAngle;

      // Draw center dot
      tft.fillCircle(screenCenterX, screenCenterY, 5, CENTER_DOT_COLOR);
    } else {
      // Normal case: just add one segment
      int startAngle = 270 + (seconds * 6) - 6;  // Previous second position
      int endAngle = startAngle + 6;             // Current second position

      // Draw the new segment
      drawSecondsArc(screenCenterX, screenCenterY, startAngle, endAngle, RING_RADIUS, RING_THICKNESS, SECOND_RING_COLOR);
    }

    prevSecond = seconds;
  }

  // Check if minute hand needs to be updated
  if (minuteAngle != prevMinuteAngle) {
    // Draw over the old minute hand with black
    if (prevMinuteX != -1 && prevMinuteY != -1) {
      // Draw a thicker black line to cover the old minute hand
      for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
          tft.drawLine(screenCenterX + i, screenCenterY + j,
                       prevMinuteX + i, prevMinuteY + j, TFT_BLACK);
        }
      }
    }

    // Calculate new minute hand position
    float minuteRad = minuteAngle * DEG_TO_RAD;
    int minuteX = screenCenterX + sin(minuteRad) * minuteHandLength;
    int minuteY = screenCenterY - cos(minuteRad) * minuteHandLength;

    // Draw the new minute hand
    tft.drawLine(screenCenterX, screenCenterY, minuteX, minuteY, MINUTE_HAND_COLOR);

    // Save new position
    prevMinuteX = minuteX;
    prevMinuteY = minuteY;
    prevMinuteAngle = minuteAngle;
  }

  // Check if hour hand needs to be updated
  if (hourAngle != prevHourAngle) {
    // Draw over the old hour hand with black
    if (prevHourX != -1 && prevHourY != -1) {
      // Draw a thicker black line to cover the old hour hand
      for (int i = -2; i <= 2; i++) {
        for (int j = -2; j <= 2; j++) {
          tft.drawLine(screenCenterX + i, screenCenterY + j,
                       prevHourX + i, prevHourY + j, TFT_BLACK);
        }
      }
    }

    // Calculate new hour hand position
    float hourRad = hourAngle * DEG_TO_RAD;
    int hourX = screenCenterX + sin(hourRad) * hourHandLength;
    int hourY = screenCenterY - cos(hourRad) * hourHandLength;

    // Draw the new hour hand
    tft.drawLine(screenCenterX, screenCenterY, hourX, hourY, HOUR_HAND_COLOR);

    // Save new position
    prevHourX = hourX;
    prevHourY = hourY;
    prevHourAngle = hourAngle;
  }

  // Redraw center dot (it might get partially erased by the hand updates)
  tft.fillCircle(screenCenterX, screenCenterY, 5, CENTER_DOT_COLOR);
}

#endif  // ARC_ANALOG_H
