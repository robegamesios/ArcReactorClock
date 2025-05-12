/*
 * theme_color_memory.h - Theme-specific LED color memory system
 * For Multi-Mode Digital Clock project
 * Remembers LED color preferences for each theme/background
 */

#ifndef THEME_COLOR_MEMORY_H
#define THEME_COLOR_MEMORY_H

#include <Arduino.h>
#include <SPIFFS.h>
#include "theme_manager.h"

// File to store theme-color mappings
#define THEME_COLOR_MAP_FILE "/bgcolors.txt"  // Changed to shorter name

// Maximum number of theme-color mappings to store
#define MAX_THEME_MAPPINGS 100
#define MAX_FILENAME_LENGTH 32

// Structure to hold a theme-color mapping with filename as identifier
struct ThemeColorMapping {
  char filename[MAX_FILENAME_LENGTH];  // Background filename for unique identification
  int colorIndex;                      // LED color index
  bool isValid;                        // Flag to indicate if mapping is valid
};

// Array to hold all theme-color mappings
ThemeColorMapping colorMappings[MAX_THEME_MAPPINGS];
int numColorMappings = 0;

// External references (defined in main sketch)
extern int currentMode;
extern int currentBgIndex;
extern String backgroundImages[];

// Function prototypes
bool saveThemeColorMappings();
bool loadThemeColorMappings();
int findThemeColorMapping(const char* filename);
void saveThemeColorPreference(const char* filename, int colorIndex);
int getThemeColorPreference(const char* filename);
void printThemeColorMappings();
void resetAllColorMappings();
bool validateColorMappings();

// Extract filename from path - handling special cases
String getFilenameFromPath(const String& path) {
  String result = path;

  // Remove path if present
  int lastSlash = result.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < result.length() - 1) {
    result = result.substring(lastSlash + 1);
  }

  // Debug output
  Serial.print("Extracted filename: '");
  Serial.print(result);
  Serial.println("'");

  return result;
}

// Load all theme-color mappings from file
bool loadThemeColorMappings() {
  // Check if mapping file exists
  if (!SPIFFS.exists(THEME_COLOR_MAP_FILE)) {
    Serial.println("No color mappings file found");
    return false;
  }

  // Open file for reading
  File file = SPIFFS.open(THEME_COLOR_MAP_FILE, "r");
  if (!file) {
    Serial.println("Failed to open color mappings file");
    return false;
  }

  // Clear existing mappings
  for (int i = 0; i < MAX_THEME_MAPPINGS; i++) {
    colorMappings[i].isValid = false;
  }
  numColorMappings = 0;

  // Read file line by line
  String line;
  while (file.available() && numColorMappings < MAX_THEME_MAPPINGS) {
    line = file.readStringUntil('\n');
    line.trim();

    // Skip empty lines
    if (line.length() == 0) {
      continue;
    }

    // Parse line format: "filename:colorIndex"
    int colonPos = line.indexOf(':');

    if (colonPos > 0 && colonPos < line.length() - 1) {
      // Extract values
      String filename = line.substring(0, colonPos);
      int colorIndex = line.substring(colonPos + 1).toInt();

      // Ensure color index is valid
      if (colorIndex >= 0 && colorIndex < COLOR_TOTAL) {
        // Store mapping
        strncpy(colorMappings[numColorMappings].filename, filename.c_str(), MAX_FILENAME_LENGTH - 1);
        colorMappings[numColorMappings].filename[MAX_FILENAME_LENGTH - 1] = '\0';  // Ensure null termination
        colorMappings[numColorMappings].colorIndex = colorIndex;
        colorMappings[numColorMappings].isValid = true;
        numColorMappings++;

        // Debug output
        Serial.print("Loaded mapping: '");
        Serial.print(filename);
        Serial.print("' -> ");
        Serial.println(colorIndex);
      }
    }
  }

  file.close();

  // Debug output
  Serial.print("Loaded ");
  Serial.print(numColorMappings);
  Serial.println(" color mappings");

  // Validate the loaded mappings
  if (!validateColorMappings()) {
    Serial.println("Some mappings were invalid and have been removed");
    saveThemeColorMappings();  // Re-save to clean up invalid entries
  }

  // Print all loaded mappings
  printThemeColorMappings();

  return (numColorMappings > 0);
}

// Save all theme-color mappings to file
bool saveThemeColorMappings() {
  // Open file for writing (recreate it every time)
  File file = SPIFFS.open(THEME_COLOR_MAP_FILE, "w");
  if (!file) {
    Serial.println("Failed to create color mappings file");
    return false;
  }

  // Write each mapping on a separate line
  int count = 0;
  for (int i = 0; i < numColorMappings; i++) {
    if (colorMappings[i].isValid) {
      file.print(colorMappings[i].filename);
      file.print(":");
      file.println(colorMappings[i].colorIndex);
      count++;
    }
  }

  file.close();

  Serial.print("Saved ");
  Serial.print(count);
  Serial.println(" color mappings");

  return true;
}

// Find the index of a theme-color mapping by filename
// Returns -1 if not found
int findThemeColorMapping(const char* filename) {
  for (int i = 0; i < numColorMappings; i++) {
    if (colorMappings[i].isValid && strcmp(colorMappings[i].filename, filename) == 0) {
      return i;
    }
  }
  return -1;
}

// Save color preference for a specific background file
void saveThemeColorPreference(const char* filename, int colorIndex) {
  // Skip if filename is empty or too long
  if (filename == NULL || strlen(filename) == 0 || strlen(filename) >= MAX_FILENAME_LENGTH) {
    Serial.println("Invalid filename for color preference");
    return;
  }

  // Validate color index
  if (colorIndex < 0 || colorIndex >= COLOR_TOTAL) {
    Serial.print("Invalid color index: ");
    Serial.println(colorIndex);
    return;
  }

  // Skip for weather mode
  if (currentMode == MODE_WEATHER) {
    return;
  }

  Serial.print("Saving color preference: File='");
  Serial.print(filename);
  Serial.print("', Color=");
  Serial.print(colorIndex);
  Serial.print(" (");
  Serial.print(ledColors[colorIndex].name);
  Serial.println(")");

  // Look for existing mapping
  int index = findThemeColorMapping(filename);

  if (index >= 0) {
    // Update existing mapping if color changed
    if (colorMappings[index].colorIndex != colorIndex) {
      colorMappings[index].colorIndex = colorIndex;
      saveThemeColorMappings();  // Save only when changed
      Serial.println("Updated existing mapping");
    } else {
      Serial.println("No change to existing mapping");
    }
  } else if (numColorMappings < MAX_THEME_MAPPINGS) {
    // Add new mapping
    strncpy(colorMappings[numColorMappings].filename, filename, MAX_FILENAME_LENGTH - 1);
    colorMappings[numColorMappings].filename[MAX_FILENAME_LENGTH - 1] = '\0';  // Ensure null termination
    colorMappings[numColorMappings].colorIndex = colorIndex;
    colorMappings[numColorMappings].isValid = true;
    numColorMappings++;
    saveThemeColorMappings();  // Save when new entry added
    Serial.println("Added new mapping");
  } else {
    Serial.println("WARNING: Maximum number of mappings reached!");
  }
}

// Get color preference for a specific background
int getThemeColorPreference(const char* filename) {
  // Skip if filename is empty
  if (filename == NULL || strlen(filename) == 0) {
    Serial.println("Empty filename, using default color");
    return getCurrentLedColor();
  }

  // Skip for weather mode
  if (currentMode == MODE_WEATHER) {
    return getCurrentLedColor();
  }

  // Look for existing mapping
  int index = findThemeColorMapping(filename);

  if (index >= 0) {
    Serial.print("Found color for '");
    Serial.print(filename);
    Serial.print("': ");
    Serial.print(colorMappings[index].colorIndex);
    Serial.print(" (");
    Serial.print(ledColors[colorMappings[index].colorIndex].name);
    Serial.println(")");
    return colorMappings[index].colorIndex;
  }

  // Return current color if no preference is found
  int currentColor = getCurrentLedColor();
  Serial.print("No color found for '");
  Serial.print(filename);
  Serial.print("', using current color: ");
  Serial.println(currentColor);
  return currentColor;
}

// Debug function to print all theme-color mappings
void printThemeColorMappings() {
  Serial.println("Background Color Mappings:");

  for (int i = 0; i < numColorMappings; i++) {
    if (colorMappings[i].isValid) {
      Serial.print("  BG: '");
      Serial.print(colorMappings[i].filename);
      Serial.print("', Color: ");
      Serial.print(colorMappings[i].colorIndex);
      Serial.print(" (");
      Serial.print(ledColors[colorMappings[i].colorIndex].name);
      Serial.println(")");
    }
  }
}

// Reset all color mappings
void resetAllColorMappings() {
  Serial.println("Resetting all color mappings...");

  // Clear all mappings
  for (int i = 0; i < MAX_THEME_MAPPINGS; i++) {
    colorMappings[i].isValid = false;
  }
  numColorMappings = 0;

  // Save empty mappings file
  saveThemeColorMappings();

  Serial.println("All color mappings reset.");
}

// Validate all color mappings
bool validateColorMappings() {
  bool isValid = true;

  for (int i = 0; i < numColorMappings; i++) {
    if (colorMappings[i].isValid) {
      // Check that filename is not empty
      if (strlen(colorMappings[i].filename) == 0) {
        Serial.print("Invalid mapping #");
        Serial.print(i);
        Serial.println(": Empty filename");
        colorMappings[i].isValid = false;
        isValid = false;
        continue;
      }

      // Check that color index is valid
      if (colorMappings[i].colorIndex < 0 || colorMappings[i].colorIndex >= COLOR_TOTAL) {
        Serial.print("Invalid mapping #");
        Serial.print(i);
        Serial.print(": Invalid color index ");
        Serial.println(colorMappings[i].colorIndex);
        colorMappings[i].isValid = false;
        isValid = false;
        continue;
      }
    }
  }

  return isValid;
}

#endif  // THEME_COLOR_MEMORY_H
