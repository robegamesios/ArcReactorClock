/*
 * config.h - Configuration settings
 * For Multi-Mode Digital Clock project
 * 
 * IMPORTANT: DO NOT check this file into version control!
 * This file contains sensitive information like API keys and passwords.
 */

#ifndef CONFIG_H
#define CONFIG_H

// WiFi settings
#define WIFI_SSID "your-wifi-ssid"        // Replace with your WiFi network name
#define WIFI_PASSWORD "your-wifi-password" // Replace with your WiFi password

// OpenWeatherMap API settings
#define WEATHER_API_KEY "your-api-key"     // Your OpenWeatherMap API key
#define WEATHER_CITY_ID 5391959            // City ID for weather (default: San Francisco, CA)
#define WEATHER_UNITS "imperial"           // Options: "imperial" for °F, "metric" for °C

// NTP Server settings
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -28800              // PST offset (-8 hours * 3600 seconds/hour)
#define DAYLIGHT_OFFSET_SEC 3600           // Daylight saving time adjustment (1 hour)

#endif // CONFIG_H
