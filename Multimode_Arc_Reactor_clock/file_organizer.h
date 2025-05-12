/*
 * file_organizer.h - Background File Organization
 * For Multi-Mode Digital Clock project
 * Organizes background files with a simplified, reliable sorting method
 */

#ifndef FILE_ORGANIZER_H
#define FILE_ORGANIZER_H

#include <Arduino.h>

// External references
extern String backgroundImages[];
extern int numBgImages;
extern int currentBgIndex;

// Extract numeric prefix from filename
int getNumericPrefix(String filename) {
  // Get just the filename without path
  if (filename.startsWith("/")) {
    filename = filename.substring(1);
  }
  
  // If it doesn't start with a digit, return a high value (for sorting)
  if (!isdigit(filename.charAt(0))) {
    return 999;
  }

  // Extract digits from the beginning
  String prefix = "";
  for (int i = 0; i < filename.length() && isdigit(filename.charAt(i)); i++) {
    prefix += filename.charAt(i);
  }
  
  // Convert to integer
  return prefix.toInt();
}

// Helper function for file type identification
bool isJpegFile(const String& filename) {
  return filename.endsWith(".jpg") || filename.endsWith(".jpeg");
}

bool isGifFile(const String& filename) {
  return filename.endsWith(".gif");
}

bool isVaultboyFile(const String& filename) {
  String lowerFilename = filename;
  lowerFilename.toLowerCase();
  return lowerFilename.indexOf("vaultboy") >= 0;
}

bool isWeatherFile(const String& filename) {
  String lowerFilename = filename;
  lowerFilename.toLowerCase();
  return lowerFilename.indexOf("weather") >= 0;
}

bool isAppleRingsFile(const String& filename) {
  String lowerFilename = filename;
  lowerFilename.toLowerCase();
  return lowerFilename.indexOf("apple_rings") >= 0;
}

// Determine file category (for sorting priority)
// 0: JPEG, 1: GIF, 2: Weather, 3: Vaultboy, 4: Apple Rings, 5: Other
int getFileCategory(const String& filename) {
  if (isAppleRingsFile(filename)) return 4;
  if (isVaultboyFile(filename)) return 3;
  if (isWeatherFile(filename)) return 2;
  if (isJpegFile(filename)) return 0;
  if (isGifFile(filename)) return 1;
  return 4;
}

// Organize background files in the desired order
void sortBackgroundImages() {
  if (numBgImages <= 1) return;

  // Use direct sorting to ensure correct order
  for (int i = 0; i < numBgImages - 1; i++) {
    for (int j = 0; j < numBgImages - i - 1; j++) {
      // Get file categories
      int cat1 = getFileCategory(backgroundImages[j]);
      int cat2 = getFileCategory(backgroundImages[j + 1]);
      
      // Sort by category first
      if (cat1 > cat2) {
        // Swap files
        String temp = backgroundImages[j];
        backgroundImages[j] = backgroundImages[j + 1];
        backgroundImages[j + 1] = temp;
      }
      // If same category, sort by numeric prefix
      else if (cat1 == cat2) {
        int prefix1 = getNumericPrefix(backgroundImages[j]);
        int prefix2 = getNumericPrefix(backgroundImages[j + 1]);
        
        if (prefix1 > prefix2) {
          // Swap files
          String temp = backgroundImages[j];
          backgroundImages[j] = backgroundImages[j + 1];
          backgroundImages[j + 1] = temp;
        }
      }
    }
  }
  
  // Debug print the sorted order
  Serial.println("Sorted background order:");
  for (int i = 0; i < numBgImages; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(backgroundImages[i]);
    Serial.print(" (Category: ");
    Serial.print(getFileCategory(backgroundImages[i]));
    Serial.print(", Prefix: ");
    Serial.print(getNumericPrefix(backgroundImages[i]));
    Serial.println(")");
  }
  
  currentBgIndex = 0;
}

// Print current background info
void printCurrentBackground() {
  if (currentBgIndex >= 0 && currentBgIndex < numBgImages) {
    Serial.print("Current background (");
    Serial.print(currentBgIndex + 1);
    Serial.print("/");
    Serial.print(numBgImages);
    Serial.print("): ");
    Serial.println(backgroundImages[currentBgIndex]);
  }
}

#endif // FILE_ORGANIZER_H
