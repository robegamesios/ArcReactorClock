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

// Update LED ring based on current weather data
void updateWeatherLEDs() {
  Serial.println("Checking if weather LED color update needed...");
  
  // First verify we have valid weather data
  if (!currentWeather.valid) {
    Serial.println("Weather data not valid, skipping LED update");
    return;
  }
  
  // Check if icon code is valid
  if (strlen(currentWeather.iconCode) < 2) {
    Serial.println("Warning: Invalid icon code, using default");
    strcpy(currentWeather.iconCode, "01d"); // Default to clear day
  }
  
  // Get the appropriate LED color based on current weather conditions
  int weatherColor = getWeatherLEDColor();
  
  // Only update if there's a change in color
  if (weatherColor != currentLedColor) {
    Serial.print("Updating LED color from ");
    Serial.print(currentLedColor);
    Serial.print(" (");
    Serial.print(getColorName(currentLedColor));
    Serial.print(") to ");
    Serial.print(weatherColor);
    Serial.print(" (");
    Serial.print(getColorName(weatherColor));
    Serial.println(")");
    
    // Update the LED color
    updateModeColorsFromLedColor(weatherColor);
    
    // Update the LEDs
    updateLEDs();
    
    // Show feedback about the color change
    showColorNameOverlay();
  } else {
    Serial.println("No change in weather LED color needed");
  }
}

// Determine the appropriate LED color based on weather conditions
int getWeatherLEDColor() {
  // First check if weather data is valid
  if (!currentWeather.valid) {
    Serial.println("Weather data invalid, keeping current LED color");
    return currentLedColor; // Keep current color if no valid weather data
  }
  
  // Debug information
  Serial.print("Current weather: ");
  Serial.print(currentWeather.description);
  Serial.print(", Icon: ");
  Serial.print(currentWeather.iconCode);
  Serial.print(", Temp: ");
  Serial.println(currentWeather.temperature);
  
  // Check weather conditions based on the icon code
  // Icon codes follow OpenWeatherMap standard (e.g., "01d", "02n", etc.)
  // First digit is the main condition, second digit is the variation
  // Last character is 'd' for day or 'n' for night
  
  // Extract just the first two digits for the condition
  char condition[3];
  strncpy(condition, currentWeather.iconCode, 2);
  condition[2] = '\0';
  
  Serial.print("Weather condition code: ");
  Serial.println(condition);
  
  // Thunderstorm (code 2xx) - use purple
  if (condition[0] == '2') {
    Serial.println("Detected thunderstorm - using purple");
    return COLOR_STORM_PURPLE;
  }
  
  // Drizzle or Rain (code 3xx or 5xx) - use blue
  else if (condition[0] == '3' || condition[0] == '5') {
    Serial.println("Detected rain - using blue");
    return COLOR_RAIN_BLUE;
  }
  
  // Snow (code 6xx) - use white
  else if (condition[0] == '6') {
    Serial.println("Detected snow - using white");
    return COLOR_SNOW_WHITE;
  }
  
  // Atmosphere - fog, mist, etc. (code 7xx) - use gray
  else if (condition[0] == '7') {
    Serial.println("Detected fog/mist - using gray");
    return COLOR_FOG_GRAY;
  }
  
  // Clear sky (code 800) - use temperature-based color
  else if (strcmp(condition, "80") == 0) {
    int tempColor = getTemperatureColor(currentWeather.temperature);
    Serial.print("Clear sky - using temperature color: ");
    Serial.println(tempColor);
    return tempColor;
  }
  
  // Clouds (code 80x) - use muted temperature-based color
  else if (condition[0] == '8') {
    int tempColor = getTemperatureColor(currentWeather.temperature);
    Serial.print("Cloudy - using temperature color: ");
    Serial.println(tempColor);
    return tempColor;
  }
  
  // Default - if none of the above match or parsing fails
  Serial.println("No weather pattern detected - keeping current color");
  return currentLedColor;
}

// Determine LED color based on temperature
int getTemperatureColor(int temperature) {
  // Print current temperature
  Serial.print("Getting color for temperature: ");
  Serial.print(temperature);
  Serial.println("°");
  
  // Simple direct mapping to colors based on temperature
  if (temperature <= TEMP_FREEZING) {  // 32°F / 0°C
    Serial.println("FREEZING temperature - using Freezing Blue");
    return COLOR_FREEZING_BLUE;
  }
  else if (temperature <= TEMP_COLD) {  // 50°F / 10°C
    Serial.println("COLD temperature - using Cold Blue");
    return COLOR_COLD_BLUE;
  }
  else if (temperature <= TEMP_COOL) {  // 65°F / 18°C
    Serial.println("COOL temperature - using Cyan");
    return COLOR_COOL_CYAN;
  }
  else if (temperature <= TEMP_COMFORT) {  // 75°F / 24°C
    Serial.println("COMFORTABLE temperature - using Green");
    return COLOR_COMFORT_GREEN;
  }
  else if (temperature <= TEMP_WARM) {  // 85°F / 29°C
    Serial.println("WARM temperature - using Yellow");
    return COLOR_WARM_YELLOW;
  }
  else if (temperature <= TEMP_HOT) {  // 95°F / 35°C
    Serial.println("HOT temperature - using Orange");
    return COLOR_HOT_ORANGE;
  }
  else {
    Serial.println("VERY HOT temperature - using Red");
    return COLOR_VERY_HOT_RED;
  }
}

// Function to force a weather LED update for testing
void forceWeatherLEDUpdate() {
  if (currentWeather.valid) {
    Serial.println("Forcing weather LED update...");
    updateWeatherLEDs();
  } else {
    Serial.println("Cannot force LED update - no valid weather data");
  }
}

// Function to force a specific temperature for testing
void testWeatherLEDColor(int testTemp) {
  Serial.print("TESTING: Setting temperature to ");
  Serial.print(testTemp);
  Serial.println("° for LED color testing");
  
  // Temporarily override the current temperature
  int savedTemp = currentWeather.temperature;
  currentWeather.temperature = testTemp;
  
  // Force weather data to be valid for testing
  currentWeather.valid = true;
  
  // Update LED color based on test temperature
  updateWeatherLEDs();
  
  // Restore original temperature
  currentWeather.temperature = savedTemp;
}

void setWeatherLEDColorDirectly() {
  if (!currentWeather.valid) {
    Serial.println("No valid weather data yet, can't set LED color");
    return;
  }
  
  Serial.print("Current temperature: ");
  Serial.print(currentWeather.temperature);
  Serial.println("°");
  
  // Get appropriate color for this temperature
  int newColor = getTemperatureColor(currentWeather.temperature);
  
  // Update LED color if different from current
  if (newColor != currentLedColor) {
    Serial.print("Changing LED color from ");
    Serial.print(currentLedColor);
    Serial.print(" to ");
    Serial.println(newColor);
    
    updateModeColorsFromLedColor(newColor);
    updateLEDs();
  } else {
    Serial.println("LED color already correct for current temperature");
  }
}

#endif // WEATHER_LED_H
