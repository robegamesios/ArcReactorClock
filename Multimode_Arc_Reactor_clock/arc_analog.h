/*
 * arc_analog.h - Arc Reactor Analog Clock Mode
 * For Multi-Mode Digital Clock project
 */

#ifndef ARC_ANALOG_H
#define ARC_ANALOG_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "utils.h"

// Function prototypes
void drawAnalogClock();
void updateAnalogClock();

// Draw full analog clock (background is drawn separately)
void drawAnalogClock() {
  // Calculate hand angles
  float secondAngle = seconds * 6; // 6 degrees per second
  float minuteAngle = minutes * 6 + (seconds * 0.1); // 6 degrees per minute + slight adjustment for seconds
  float hourAngle = hours * 30 + (minutes * 0.5); // 30 degrees per hour + adjustment for minutes
  
  // Get the hand lengths based on clock radius
  int secondHandLength = screenRadius * 0.8;
  int minuteHandLength = screenRadius * 0.7;
  int hourHandLength = screenRadius * 0.5;
  
  // Draw hour markers (12 dots around the clock)
  for (int i = 0; i < 12; i++) {
    float angle = i * 30 * DEG_TO_RAD; // 30 degrees per hour mark
    int x = screenCenterX + sin(angle) * (screenRadius * 0.8);
    int y = screenCenterY - cos(angle) * (screenRadius * 0.8);
    
    tft.fillCircle(x, y, 3, TFT_CYAN);
  }
  
  // Draw hour hand
  float hourRad = hourAngle * DEG_TO_RAD;
  int hourX = screenCenterX + sin(hourRad) * hourHandLength;
  int hourY = screenCenterY - cos(hourRad) * hourHandLength;
  tft.drawLine(screenCenterX, screenCenterY, hourX, hourY, TFT_WHITE);
  
  // Draw minute hand
  float minuteRad = minuteAngle * DEG_TO_RAD;
  int minuteX = screenCenterX + sin(minuteRad) * minuteHandLength;
  int minuteY = screenCenterY - cos(minuteRad) * minuteHandLength;
  tft.drawLine(screenCenterX, screenCenterY, minuteX, minuteY, TFT_YELLOW);
  
  // Draw second hand
  float secondRad = secondAngle * DEG_TO_RAD;
  int secondX = screenCenterX + sin(secondRad) * secondHandLength;
  int secondY = screenCenterY - cos(secondRad) * secondHandLength;
  tft.drawLine(screenCenterX, screenCenterY, secondX, secondY, TFT_RED);
  
  // Draw center dot
  tft.fillCircle(screenCenterX, screenCenterY, 5, TFT_CYAN);
}

// Update analog clock display (clear center and redraw)
void updateAnalogClock() {
  // Define background color from arc_digital.h
  extern uint16_t bgColor;
  
  // Clear center area (keep the background elements)
  tft.fillCircle(screenCenterX, screenCenterY, screenRadius * 0.65, bgColor);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.65, TFT_CYAN);
  
  // Draw analog clock
  drawAnalogClock();
}

#endif // ARC_ANALOG_H