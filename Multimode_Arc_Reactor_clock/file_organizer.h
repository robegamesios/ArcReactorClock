/*
 * file_organizer.h - Background File Organization
 * For Multi-Mode Digital Clock project
 * Organizes background files in specified order: 
 * JPEGs first (Iron Man at beginning), then GIFs, then special themes
 */

#ifndef FILE_ORGANIZER_H
#define FILE_ORGANIZER_H

#include <Arduino.h>

// External references - we don't use MAX_BACKGROUNDS anymore
extern String backgroundImages[];
extern int numBgImages;
extern int currentBgIndex;

// Function to check if filename contains a specific substring (case insensitive)
bool filenameContains(const String& filename, const char* substr) {
  String lowerFilename = filename;
  lowerFilename.toLowerCase();
  return lowerFilename.indexOf(substr) >= 0;
}

// Sort background images in the preferred order
void sortBackgroundImages() {
  if (numBgImages <= 1) return;

  // We'll use the actual size of the array instead of MAX_BACKGROUNDS
  String* sortedImages = new String[numBgImages];
  int sortedCount = 0;

  // STEP 1: Find and place Iron Man image first (if exists)
  for (int i = 0; i < numBgImages; i++) {
    if (filenameContains(backgroundImages[i], "ironman") && (backgroundImages[i].endsWith(".jpg") || backgroundImages[i].endsWith(".jpeg"))) {
      sortedImages[sortedCount++] = backgroundImages[i];
      backgroundImages[i] = "";  // Mark as processed
      break;
    }
  }

  // STEP 2: Add remaining JPEG files
  for (int i = 0; i < numBgImages; i++) {
    if (backgroundImages[i] != "" && (backgroundImages[i].endsWith(".jpg") || backgroundImages[i].endsWith(".jpeg")) && !filenameContains(backgroundImages[i], "weather")) {  // Exclude weather JPEGs
      sortedImages[sortedCount++] = backgroundImages[i];
      backgroundImages[i] = "";  // Mark as processed
    }
  }

  // STEP 3: Add regular GIF files (exclude vaultboy.gif and weather GIFs)
  for (int i = 0; i < numBgImages; i++) {
    if (backgroundImages[i] != "" && backgroundImages[i].endsWith(".gif") && !filenameContains(backgroundImages[i], "vaultboy") && !filenameContains(backgroundImages[i], "weather")) {
      sortedImages[sortedCount++] = backgroundImages[i];
      backgroundImages[i] = "";  // Mark as processed
    }
  }

  // STEP 4: Add weather-themed files (both JPEG and GIF)
  for (int i = 0; i < numBgImages; i++) {
    if (backgroundImages[i] != "" && filenameContains(backgroundImages[i], "weather")) {
      sortedImages[sortedCount++] = backgroundImages[i];
      backgroundImages[i] = "";  // Mark as processed
    }
  }

  // STEP 5: Add Pip-Boy theme (vaultboy.gif) last
  for (int i = 0; i < numBgImages; i++) {
    if (backgroundImages[i] != "" && filenameContains(backgroundImages[i], "vaultboy")) {
      sortedImages[sortedCount++] = backgroundImages[i];
      backgroundImages[i] = "";  // Mark as processed
    }
  }

  // STEP 6: Add any remaining files that didn't match our categories
  for (int i = 0; i < numBgImages; i++) {
    if (backgroundImages[i] != "") {
      sortedImages[sortedCount++] = backgroundImages[i];
    }
  }

  // Copy sorted array back to backgroundImages
  for (int i = 0; i < sortedCount; i++) {
    backgroundImages[i] = sortedImages[i];
  }

  // Clean up our temporary array
  delete[] sortedImages;

  // Debug print the sorted order
  Serial.println("Sorted background order:");
  for (int i = 0; i < numBgImages; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(backgroundImages[i]);
  }

  currentBgIndex = 0;
}

// Add debug function to print current background when cycling
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

#endif  // FILE_ORGANIZER_H
