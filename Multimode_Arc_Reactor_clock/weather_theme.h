/*
 * weather_theme.h - Weather Display Mode
 * For Multi-Mode Digital Clock project
 * Features current weather data from OpenWeatherMap API
 */

#ifndef WEATHER_THEME_H
#define WEATHER_THEME_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "utils.h"
#include "weather_data.h"
#include "weather_led.h"

// Weather display mode ID
#define MODE_WEATHER 4  // Weather mode is mode #4

// OpenWeatherMap API settings
extern const char* weatherApiKey;
extern const long weatherCityId;
extern char weatherUnits[10];

// External references
extern bool isClockHidden;
extern unsigned long lastWeatherUpdate;
extern const unsigned long weatherUpdateInterval;

// Colors for weather theme
#define WEATHER_BG TFT_BLACK    // Black background
#define WEATHER_TEXT TFT_WHITE  // White for text

// Previous time values to track changes
int prevWeatherHours = -1, prevWeatherMinutes = -1, prevWeatherSeconds = -1;

// Arc/Circle settings for seconds indicator
#define WEATHER_SECONDS_RADIUS 115   // Radius for seconds circle
#define WEATHER_SECONDS_THICKNESS 4  // Thickness of the seconds ring

// Weather icon position
#define WEATHER_ICON_X 55
#define WEATHER_ICON_Y 130

// Function prototypes
void initWeatherTheme();
void drawWeatherInterface();
void updateWeatherTime();
void updateWeatherData();
void drawWeatherIcon();
void cleanupWeatherMode();
bool fetchWeatherData();
void drawWeatherSecondsIndicator();
void updateWeatherSecondsIndicator();
void updateWeatherIcon();
void drawDegreeSymbol(int x, int y, int size, uint16_t color);

// Initialize the weather theme
void initWeatherTheme() {
  // Reset previous values
  prevWeatherHours = -1;
  prevWeatherMinutes = -1;
  prevWeatherSeconds = -1;

  // Fetch weather data
  updateWeatherData();
}

// Fetch weather data from OpenWeatherMap API
bool fetchWeatherData() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  WiFiClient client;
  HTTPClient http;

  // Construct the URL using city ID
  char url[150];
  snprintf(url, sizeof(url),
           "http://api.openweathermap.org/data/2.5/weather?id=%ld&units=%s&appid=%s",
           weatherCityId, weatherUnits, weatherApiKey);

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  // Use ArduinoJson to parse the response
  const size_t capacity = JSON_OBJECT_SIZE(5) + JSON_ARRAY_SIZE(1) + 200;
  DynamicJsonDocument doc(capacity);

  String payload = http.getString();
  DeserializationError error = deserializeJson(doc, payload);
  
  if (error) {
    http.end();
    return false;
  }

  // Extract weather data
  if (doc.containsKey("weather") && doc["weather"].is<JsonArray>() && doc["weather"].size() > 0) {
    strncpy(currentWeather.description, doc["weather"][0]["description"].as<const char*>(), sizeof(currentWeather.description) - 1);
    currentWeather.description[sizeof(currentWeather.description) - 1] = '\0';

    strncpy(currentWeather.iconCode, doc["weather"][0]["icon"].as<const char*>(), sizeof(currentWeather.iconCode) - 1);
    currentWeather.iconCode[sizeof(currentWeather.iconCode) - 1] = '\0';
  }

  if (doc.containsKey("main")) {
    currentWeather.temperature = (int8_t)doc["main"]["temp"].as<float>();
    currentWeather.feelsLike = (int8_t)doc["main"]["feels_like"].as<float>();
    currentWeather.tempMin = (int8_t)doc["main"]["temp_min"].as<float>();
    currentWeather.tempMax = (int8_t)doc["main"]["temp_max"].as<float>();
    currentWeather.humidity = (uint8_t)doc["main"]["humidity"].as<int>();
  }

  if (doc.containsKey("wind")) {
    currentWeather.windSpeed = (uint8_t)doc["wind"]["speed"].as<float>();
  }

  currentWeather.lastUpdate = millis();
  currentWeather.valid = true;

  http.end();
  return true;
}

// Update weather data if needed
void updateWeatherData() {
  unsigned long currentMillis = millis();

  // Check if it's time for an update
  if (!currentWeather.valid || (currentMillis - lastWeatherUpdate >= weatherUpdateInterval)) {
    bool dataUpdated = fetchWeatherData();

    if (dataUpdated) {
      lastWeatherUpdate = currentMillis;
      setWeatherLEDColorDirectly();
    }
  }
}

// Placeholder for animated weather icon updates
void updateWeatherIcon() {
  // Static icons, nothing to update
}

// Cleanup resources when switching from Weather mode
void cleanupWeatherMode() {
  // Nothing to clean up in this version
}

// Draw a proper degree symbol
void drawDegreeSymbol(int x, int y, int size, uint16_t color) {
  int radius = size * 2;
  tft.drawCircle(x, y, radius, color);
}

// Draw a static weather icon based on the icon code
void drawWeatherIcon() {
  // Set icon position
  int iconX = WEATHER_ICON_X;
  int iconY = WEATHER_ICON_Y;

  // Clear area for the icon
  tft.fillRect(iconX - 40, iconY - 40, 80, 80, WEATHER_BG);

  // Check if we have a valid icon code
  bool isDay = true;
  if (strlen(currentWeather.iconCode) >= 3) {
    isDay = currentWeather.iconCode[2] == 'd';
  }

  // Draw weather icon based on icon code
  // Clear or Few Clouds
  if (strcmp(currentWeather.iconCode, "01d") == 0 || strcmp(currentWeather.iconCode, "01n") == 0) {
    // Clear sky
    if (isDay) {
      // Sun
      tft.fillCircle(iconX, iconY, 20, TFT_YELLOW);
    } else {
      // Moon
      tft.fillCircle(iconX, iconY, 20, TFT_LIGHTGREY);
      tft.fillCircle(iconX + 10, iconY - 10, 20, WEATHER_BG);  // Bite out of the moon
    }
  }
  // Few Clouds
  else if (strcmp(currentWeather.iconCode, "02d") == 0 || strcmp(currentWeather.iconCode, "02n") == 0) {
    // Few clouds
    if (isDay) {
      // Sun with cloud
      tft.fillCircle(iconX - 10, iconY - 5, 12, TFT_YELLOW);          // Sun
      tft.fillRoundRect(iconX - 5, iconY, 30, 15, 8, TFT_LIGHTGREY);  // Cloud
    } else {
      // Moon with cloud
      tft.fillCircle(iconX - 10, iconY - 5, 12, TFT_LIGHTGREY);       // Moon
      tft.fillCircle(iconX - 5, iconY - 10, 8, WEATHER_BG);           // Bite out of moon
      tft.fillRoundRect(iconX - 5, iconY, 30, 15, 8, TFT_LIGHTGREY);  // Cloud
    }
  }
  // Clouds
  else if (strcmp(currentWeather.iconCode, "03d") == 0 || strcmp(currentWeather.iconCode, "03n") == 0 || 
           strcmp(currentWeather.iconCode, "04d") == 0 || strcmp(currentWeather.iconCode, "04n") == 0) {
    // Clouds
    tft.fillRoundRect(iconX - 25, iconY - 10, 50, 20, 10, TFT_LIGHTGREY);  // Main cloud
    tft.fillRoundRect(iconX - 15, iconY - 20, 40, 15, 8, TFT_WHITE);       // Top cloud
  }
  // Rain
  else if (strcmp(currentWeather.iconCode, "09d") == 0 || strcmp(currentWeather.iconCode, "09n") == 0 || 
           strcmp(currentWeather.iconCode, "10d") == 0 || strcmp(currentWeather.iconCode, "10n") == 0) {
    // Rain
    tft.fillRoundRect(iconX - 25, iconY - 15, 50, 20, 10, TFT_LIGHTGREY);  // Cloud
    // Raindrops
    for (int i = -15; i <= 15; i += 10) {
      tft.fillRoundRect(iconX + i, iconY + 10, 3, 15, 2, 0x5E9F);  // Light blue raindrops
    }
  }
  // Thunderstorm
  else if (strcmp(currentWeather.iconCode, "11d") == 0 || strcmp(currentWeather.iconCode, "11n") == 0) {
    // Thunderstorm
    tft.fillRoundRect(iconX - 25, iconY - 15, 50, 20, 10, TFT_LIGHTGREY);  // Cloud
    // Lightning bolt
    tft.fillTriangle(iconX - 5, iconY + 5, iconX + 10, iconY + 15, iconX - 10, iconY + 20, TFT_YELLOW);
    tft.fillTriangle(iconX - 10, iconY + 20, iconX + 10, iconY + 15, iconX, iconY + 35, TFT_YELLOW);
  }
  // Snow
  else if (strcmp(currentWeather.iconCode, "13d") == 0 || strcmp(currentWeather.iconCode, "13n") == 0) {
    // Snow
    tft.fillRoundRect(iconX - 25, iconY - 15, 50, 20, 10, TFT_LIGHTGREY);  // Cloud
    // Snowflakes
    for (int i = -15; i <= 15; i += 10) {
      tft.fillCircle(iconX + i, iconY + 15, 5, TFT_WHITE);
    }
  }
  // Mist/Fog
  else if (strcmp(currentWeather.iconCode, "50d") == 0 || strcmp(currentWeather.iconCode, "50n") == 0) {
    // Mist/Fog
    for (int i = -15; i <= 15; i += 7) {
      tft.drawLine(iconX - 25, iconY + i, iconX + 25, iconY + i, TFT_LIGHTGREY);
    }
  }
  // Unknown/Default
  else {
    // Unknown weather
    tft.fillRoundRect(iconX - 25, iconY - 15, 50, 30, 8, TFT_LIGHTGREY);

    // Question mark
    tft.setTextColor(WEATHER_TEXT);
    tft.setTextSize(3);
    tft.setCursor(iconX - 10, iconY - 10);
    tft.print("?");
  }
}

// Draw the initial Weather interface
void drawWeatherInterface() {
  // Clear the display
  tft.fillScreen(WEATHER_BG);

  // Draw day of week at top
  tft.setTextSize(2);
  tft.setTextColor(WEATHER_TEXT);
  int dayWidth = dayOfWeek.length() * 12;
  tft.setCursor(screenCenterX - (dayWidth / 2), 25);
  tft.println(dayOfWeek);

  // Draw date
  tft.setTextSize(2);
  char dateStr[20];
  sprintf(dateStr, "%02d.%02d.%04d", day, month, year);
  int dateWidth = strlen(dateStr) * 12;
  tft.setCursor(screenCenterX - (dateWidth / 2), 50);
  tft.println(dateStr);

  // Display weather information
  if (currentWeather.valid) {
    // Draw the static weather icon
    drawWeatherIcon();

    // Draw weather description below day/date
    tft.setTextSize(1);
    char desc[24];
    strncpy(desc, currentWeather.description, sizeof(desc));
    desc[sizeof(desc) - 1] = '\0';
    if (strlen(desc) > 0) {
      desc[0] = toupper(desc[0]);
    }

    // Center and print description
    int descWidth = strlen(desc) * 6;
    tft.setCursor(screenCenterX - (descWidth / 2), 70);
    tft.print(desc);

    // Draw temperature information
    tft.setTextSize(3);

    // Current temperature
    char tempStr[10];
    sprintf(tempStr, "%d", currentWeather.temperature);
    int tempX = 100;
    tft.setCursor(tempX, 90);
    tft.print(tempStr);

    // Draw a custom degree symbol
    int digitWidth = 16;  // Approximate width of a digit
    int textWidth = strlen(tempStr) * digitWidth;
    int degreeX = tempX + textWidth + 10;
    int degreeY = 90 + 6;
    drawDegreeSymbol(degreeX, degreeY, 2, WEATHER_TEXT);

    // Draw the unit
    tft.setTextSize(2);
    tft.setCursor(degreeX + 11, 90);
    tft.print(weatherUnits[0] == 'i' ? "F" : "C");

    // "Feels like" temperature
    tft.setTextSize(2);
    sprintf(tempStr, "Feels: %d", currentWeather.feelsLike);
    tft.setCursor(100, 115);
    tft.print(tempStr);
    degreeX = 110 + strlen(tempStr) * 12 - 4;
    degreeY = 115 + 4;
    drawDegreeSymbol(degreeX, degreeY, 1, WEATHER_TEXT);

    // High temperature
    sprintf(tempStr, "High: %d", currentWeather.tempMax);
    tft.setCursor(100, 135);
    tft.print(tempStr);
    degreeX = 110 + strlen(tempStr) * 12 - 4;
    degreeY = 135 + 4;
    drawDegreeSymbol(degreeX, degreeY, 1, WEATHER_TEXT);

    // Low temperature
    sprintf(tempStr, "Low: %d", currentWeather.tempMin);
    tft.setCursor(100, 155);
    tft.print(tempStr);
    degreeX = 110 + strlen(tempStr) * 12 - 4;
    degreeY = 155 + 4;
    drawDegreeSymbol(degreeX, degreeY, 1, WEATHER_TEXT);
  } else {
    // If no weather data available yet
    tft.setTextSize(2);
    tft.setCursor(70, 110);
    tft.println("Loading...");
  }

  tft.setTextSize(2);

  // Draw the time
  char timeStr[10];
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d:%02d %s", displayHours, minutes, hours >= 12 ? "PM" : "AM");
  int timeWidth = strlen(timeStr) * 12;
  tft.setCursor(screenCenterX - (timeWidth / 2), 195);
  tft.println(timeStr);

  // Draw seconds indicator ring
  drawWeatherSecondsIndicator();
}

// Draw the seconds indicator as an outer ring
void drawWeatherSecondsIndicator() {
  // Get the second ring color
  uint16_t secondRingColor = getCurrentSecondRingColor();

  // Draw the seconds as segments around the outer edge
  for (int i = 0; i < seconds; i++) {
    float startAngle = (i * 6 - 90) * DEG_TO_RAD;
    float endAngle = ((i + 1) * 6 - 90) * DEG_TO_RAD;

    // Calculate segment positions
    int x1 = screenCenterX + cos(startAngle) * WEATHER_SECONDS_RADIUS;
    int y1 = screenCenterY + sin(startAngle) * WEATHER_SECONDS_RADIUS;
    int x2 = screenCenterX + cos(endAngle) * WEATHER_SECONDS_RADIUS;
    int y2 = screenCenterY + sin(endAngle) * WEATHER_SECONDS_RADIUS;

    // Draw a line segment for each second
    tft.drawLine(x1, y1, x2, y2, secondRingColor);

    // Make it thicker
    for (int t = 1; t < WEATHER_SECONDS_THICKNESS; t++) {
      int x1Inner = screenCenterX + cos(startAngle) * (WEATHER_SECONDS_RADIUS - t);
      int y1Inner = screenCenterY + sin(startAngle) * (WEATHER_SECONDS_RADIUS - t);
      int x2Inner = screenCenterX + cos(endAngle) * (WEATHER_SECONDS_RADIUS - t);
      int y2Inner = screenCenterY + sin(endAngle) * (WEATHER_SECONDS_RADIUS - t);

      tft.drawLine(x1Inner, y1Inner, x2Inner, y2Inner, secondRingColor);
    }
  }
}

// Update just the seconds indicator ring
void updateWeatherSecondsIndicator() {
  // Only update if seconds changed
  static int lastSecond = -1;
  if (seconds == lastSecond) return;

  // Get the second ring color
  uint16_t secondRingColor = getCurrentSecondRingColor();

  // If seconds reset to 0, clear screen and redraw everything
  if (seconds == 0) {
    // Signal a full refresh
    needClockRefresh = true;
    lastSecond = seconds;
    return;
  }

  // Just add the new second segment
  float startAngle = ((seconds - 1) * 6 - 90) * DEG_TO_RAD;
  float endAngle = (seconds * 6 - 90) * DEG_TO_RAD;

  // Calculate segment positions
  int x1 = screenCenterX + cos(startAngle) * WEATHER_SECONDS_RADIUS;
  int y1 = screenCenterY + sin(startAngle) * WEATHER_SECONDS_RADIUS;
  int x2 = screenCenterX + cos(endAngle) * WEATHER_SECONDS_RADIUS;
  int y2 = screenCenterY + sin(endAngle) * WEATHER_SECONDS_RADIUS;

  // Draw a line segment for the new second
  tft.drawLine(x1, y1, x2, y2, secondRingColor);

  // Make it thicker
  for (int t = 1; t < WEATHER_SECONDS_THICKNESS; t++) {
    int x1Inner = screenCenterX + cos(startAngle) * (WEATHER_SECONDS_RADIUS - t);
    int y1Inner = screenCenterY + sin(startAngle) * (WEATHER_SECONDS_RADIUS - t);
    int x2Inner = screenCenterX + cos(endAngle) * (WEATHER_SECONDS_RADIUS - t);
    int y2Inner = screenCenterY + sin(endAngle) * (WEATHER_SECONDS_RADIUS - t);

    tft.drawLine(x1Inner, y1Inner, x2Inner, y2Inner, secondRingColor);
  }

  lastSecond = seconds;
}

// Update the time display in weather mode
void updateWeatherTime() {
  // Check if time components have changed
  bool hoursChanged = (hours != prevWeatherHours);
  bool minutesChanged = (minutes != prevWeatherMinutes);

  // Only update the time display if hours or minutes changed AND clock is not hidden
  if ((hoursChanged || minutesChanged) && !isClockHidden) {
    // Clear the time area
    tft.fillRect(screenCenterX - 70, 195, 140, 15, WEATHER_BG);

    // Draw updated time
    tft.setTextSize(2);
    tft.setTextColor(WEATHER_TEXT);

    char timeStr[10];
    int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
    sprintf(timeStr, "%02d:%02d %s", displayHours, minutes, hours >= 12 ? "PM" : "AM");
    int timeWidth = strlen(timeStr) * 12;
    tft.setCursor(screenCenterX - (timeWidth / 2), 195);
    tft.println(timeStr);

    // Update previous values
    prevWeatherHours = hours;
    prevWeatherMinutes = minutes;
  } else if (isClockHidden && (hoursChanged || minutesChanged)) {
    // If clock is hidden but time changed, still update our tracking variables
    prevWeatherHours = hours;
    prevWeatherMinutes = minutes;
  }

  // Always update the seconds indicator, regardless of whether time is hidden
  updateWeatherSecondsIndicator();
}

#endif  // WEATHER_THEME_H
