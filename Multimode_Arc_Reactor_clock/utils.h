/*
 * Utils.h - Common utility functions and definitions
 * For Multi-Mode Digital Clock project
 */

#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// Colors
#define PIP_GREEN 0x07E0 // Bright green for Pip-Boy mode
#define PIP_BLACK 0x0000

// Function declaration
extern void updateTimeAndDate();

// References to external variables that are defined in the main sketch
extern TFT_eSPI tft;
extern int screenCenterX;
extern int screenCenterY;
extern int screenRadius;
extern int hours, minutes, seconds;
extern int day, month, year;
extern String dayOfWeek;
extern bool is24Hour;

// Update time and date from NTP or manual increment
void updateTimeAndDate() {
  // Get current time and date if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)) {
      hours = timeinfo.tm_hour;
      minutes = timeinfo.tm_min;
      seconds = timeinfo.tm_sec;
      
      day = timeinfo.tm_mday;
      month = timeinfo.tm_mon + 1; // tm_mon is 0-11
      year = timeinfo.tm_year + 1900; // tm_year is years since 1900
      
      // Get day of week
      switch(timeinfo.tm_wday) {
        case 0: dayOfWeek = "SUNDAY"; break;
        case 1: dayOfWeek = "MONDAY"; break;
        case 2: dayOfWeek = "TUESDAY"; break;
        case 3: dayOfWeek = "WEDNESDAY"; break;
        case 4: dayOfWeek = "THURSDAY"; break;
        case 5: dayOfWeek = "FRIDAY"; break;
        case 6: dayOfWeek = "SATURDAY"; break;
      }
    }
  } else {
    // Increment time manually if no WiFi
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
          // Here we could also increment the date, but keeping it simple
        }
      }
    }
  }
}

#endif // UTILS_H
