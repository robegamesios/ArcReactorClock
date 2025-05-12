/*
 * simple_storage.h - Simplified settings storage
 * For Multi-Mode Digital Clock project
 */

#ifndef SIMPLE_STORAGE_H
#define SIMPLE_STORAGE_H

#include <Arduino.h>
#include <SPIFFS.h>

// Settings file path
#define SETTINGS_FILE "/settings.txt"

// Function to save settings to a file
bool saveSettingsToFile(int bgIndex, int clockMode, int vertPos, int ledColor) {
  // Create a buffer for settings
  char buffer[100];
  sprintf(buffer, "%d,%d,%d,%d", bgIndex, clockMode, vertPos, ledColor);

  // Open file for writing
  File file = SPIFFS.open(SETTINGS_FILE, "w");
  if (!file) {
    return false;
  }

  // Write settings
  size_t written = file.print(buffer);
  file.close();

  return (written > 0);
}

// Function to load settings from file
bool loadSettingsFromFile(int *bgIndex, int *clockMode, int *vertPos, int *ledColor) {
  // Check if settings file exists
  if (!SPIFFS.exists(SETTINGS_FILE)) {
    return false;
  }

  // Open file for reading
  File file = SPIFFS.open(SETTINGS_FILE, "r");
  if (!file) {
    return false;
  }

  // Read settings
  String contents = file.readString();
  file.close();

  if (contents.length() < 3) {
    return false;
  }

  // Parse the settings string
  int values[4] = { 0 };
  int valueIndex = 0;
  int lastCommaIndex = -1;

  // Find commas and extract values
  for (int i = 0; i < contents.length() && valueIndex < 4; i++) {
    if (contents.charAt(i) == ',' || i == contents.length() - 1) {
      // Extract value between commas
      int endPos = (i == contents.length() - 1) ? i + 1 : i;
      String valueStr = contents.substring(lastCommaIndex + 1, endPos);
      values[valueIndex] = valueStr.toInt();
      valueIndex++;
      lastCommaIndex = i;
    }
  }

  // Check if we got all four values
  if (valueIndex < 4) {
    return false;
  }

  // Return the values
  *bgIndex = values[0];
  *clockMode = values[1];
  *vertPos = values[2];
  *ledColor = values[3];

  return true;
}

#endif  // SIMPLE_STORAGE_H
