/*
 * weather_led.h - Weather-based LED color mapping
 * For Multi-Mode Digital Clock project
 * Features automatic LED ring color changes based on weather conditions
 */

#ifndef WEATHER_LED_H
#define WEATHER_LED_H

#include <Arduino.h>
#include "led_controls.h"
#include "theme_manager.h"
#include "weather_data.h"

// Add this for proper reference to weatherUnits from weather_theme.h
extern char weatherUnits[];

// Temperature ranges in selected unit (F or C)
// These are used to determine LED color in "clear" conditions
#define TEMP_FREEZING  32   // 32°F / 0°C
#define TEMP_COLD      50   // 50°F / 10°C
#define TEMP_COOL      65   // 65°F / 18°C 
#define TEMP_COMFORT   75   // 75°F / 24°C
#define TEMP_WARM      85   // 85°F / 29°C
#define TEMP_HOT       95   // 95°F / 35°C

// Function prototypes
void updateWeatherLEDs();
int getWeatherLEDColor();
int getTemperatureColor(int temperature);
void forceWeatherLEDUpdate();
void setWeatherLEDColorDirectly();

// Update LED ring based on current weather data
void updateWeatherLEDs() {
  // First verify we have valid weather data
  if (!currentWeather.valid) {
    return;
  }
  
  // Check if icon code is valid
  if (strlen(currentWeather.iconCode) < 2) {
    strcpy(currentWeather.iconCode, "01d"); // Default to clear day
  }
  
  // Get the appropriate LED color based on current weather conditions
  int weatherColor = getWeatherLEDColor();
  
  // Only update if there's a change in color
  if (weatherColor != currentLedColor) {
    // Update the LED color
    updateModeColorsFromLedColor(weatherColor);
    
    // Update the LEDs
    updateLEDs();
    
    // Show feedback about the color change
    showColorNameOverlay();
  }
}

// Determine the appropriate LED color based on weather conditions
int getWeatherLEDColor() {
  // First check if weather data is valid
  if (!currentWeather.valid) {
    return currentLedColor; // Keep current color if no valid weather data
  }
  
  // Extract just the first two digits for the condition
  char condition[3];
  strncpy(condition, currentWeather.iconCode, 2);
  condition[2] = '\0';
  
  // Thunderstorm (code 2xx) - use purple
  if (condition[0] == '2') {
    return COLOR_STORM_PURPLE;
  }
  
  // Drizzle or Rain (code 3xx or 5xx) - use blue
  else if (condition[0] == '3' || condition[0] == '5') {
    return COLOR_RAIN_BLUE;
  }
  
  // Snow (code 6xx) - use white
  else if (condition[0] == '6') {
    return COLOR_SNOW_WHITE;
  }
  
  // Atmosphere - fog, mist, etc. (code 7xx) - use gray
  else if (condition[0] == '7') {
    return COLOR_FOG_GRAY;
  }
  
  // Clear sky (code 800) - use temperature-based color
  else if (strcmp(condition, "80") == 0) {
    return getTemperatureColor(currentWeather.temperature);
  }
  
  // Clouds (code 80x) - use muted temperature-based color
  else if (condition[0] == '8') {
    return getTemperatureColor(currentWeather.temperature);
  }
  
  // Default - if none of the above match or parsing fails
  return currentLedColor;
}

// Determine LED color based on temperature
int getTemperatureColor(int temperature) {
  // Simple direct mapping to colors based on temperature
  if (temperature <= TEMP_FREEZING) {  // 32°F / 0°C
    return COLOR_FREEZING_BLUE;
  }
  else if (temperature <= TEMP_COLD) {  // 50°F / 10°C
    return COLOR_COLD_BLUE;
  }
  else if (temperature <= TEMP_COOL) {  // 65°F / 18°C
    return COLOR_COOL_CYAN;
  }
  else if (temperature <= TEMP_COMFORT) {  // 75°F / 24°C
    return COLOR_COMFORT_GREEN;
  }
  else if (temperature <= TEMP_WARM) {  // 85°F / 29°C
    return COLOR_WARM_YELLOW;
  }
  else if (temperature <= TEMP_HOT) {  // 95°F / 35°C
    return COLOR_HOT_ORANGE;
  }
  else {
    return COLOR_VERY_HOT_RED;
  }
}

// Function to force a weather LED update for testing
void forceWeatherLEDUpdate() {
  if (currentWeather.valid) {
    updateWeatherLEDs();
  }
}

// Function to set weather LED color directly
void setWeatherLEDColorDirectly() {
  if (!currentWeather.valid) {
    return;
  }
  
  // Get appropriate color for this temperature
  int newColor = getTemperatureColor(currentWeather.temperature);
  
  // Update LED color if different from current
  if (newColor != currentLedColor) {
    updateModeColorsFromLedColor(newColor);
    updateLEDs();
  }
}

#endif // WEATHER_LED_H
