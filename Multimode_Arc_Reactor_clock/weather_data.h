/*
 * weather_data.h - Shared weather data structures
 * For Multi-Mode Digital Clock project
 */

#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

#include <Arduino.h>

// Minimal weather data structure
struct WeatherData {
  char description[24];
  char iconCode[4];
  int8_t temperature;
  int8_t feelsLike;
  int8_t tempMin;
  int8_t tempMax;
  uint8_t humidity;
  uint8_t windSpeed;
  unsigned long lastUpdate;
  bool valid;
};

// Declare the shared weather data instance
extern WeatherData currentWeather;

#endif // WEATHER_DATA_H