/*
 * Combined Digital Clock with Multiple Themes and JPEG Support
 * Includes:
 * 1. Arc Reactor Digital Mode (blue) with JPEG background
 * 2. Arc Reactor Analog Mode (blue) with JPEG background
 * 3. Pip-Boy 3000 Mode (green)
 * Button press cycles through the clock faces
 */

#include <EEPROM.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>
#include <time.h>
#include <SPIFFS.h>  // For file system access

// Include TJpgDec library for JPEG decoding
#include <TJpg_Decoder.h>

// Include custom header files for different clock modes
#include "utils.h"
#include "led_controls.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"
#include "theme_persistence.h"

#define EEPROM_SIZE 16            // Size of EEPROM allocation
#define THEME_ADDRESS 0           // Address to store theme ID
#define MODE_ADDRESS 4            // Address to store current mode
#define VALID_FLAG_ADDRESS 8      // Address for valid settings flag
#define VALID_SETTINGS_FLAG 0xAA  // Flag value indicating valid settings

// Theme IDs - simple integers instead of hashes
#define THEME_ID_DEFAULT 0
#define THEME_ID_IRONMAN 1
#define THEME_ID_HULK 2
#define THEME_ID_CAPTAIN 3
#define THEME_ID_THOR 4
#define THEME_ID_BLACK_WIDOW 5
#define THEME_ID_SPIDERMAN 6

// Define pins for NeoPixel LED ring
#define LED_PIN 21
#define NUMPIXELS 35

// Button pin for mode switching
#define BUTTON_PIN 22

// Clock modes
#define MODE_ARC_DIGITAL 0  // Arc Reactor digital mode
#define MODE_ARC_ANALOG 1   // Arc Reactor analog mode
#define MODE_PIPBOY 2       // Pip-Boy theme
#define MODE_TOTAL 3        // Total number of modes

// Initialize NeoPixel library
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// TFT Display settings
TFT_eSPI tft = TFT_eSPI();

// WiFi settings - enter your credentials here
const char* ssid = "SSID";          // Enter your WiFi network name
const char* password = "PASSWORD";  // Enter your WiFi password

// Time settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;    // PST offset (-8 hours * 3600 seconds/hour)
const int daylightOffset_sec = 3600;  // Daylight saving time adjustment (1 hour)

// Display variables
int screenCenterX;
int screenCenterY;
int screenRadius;

// Time and date variables
int hours = 12, minutes = 0, seconds = 0;
int day = 1, month = 1, year = 2025;
String dayOfWeek = "WEDNESDAY";
bool is24Hour = false;  // Use 12-hour format by default
bool needClockRefresh = false;

// Mode variable
int currentMode = MODE_ARC_DIGITAL;  // Start with Arc Reactor digital mode

// Timing variables
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 300;  // Debounce time in milliseconds

// Function declarations
void cleanupPipBoyMode();
void updatePipBoyGif();
void checkForImageFiles();
bool loadImageList();
void saveCurrentThemeAndMode(int themeID = THEME_ID_DEFAULT);

// Array to store available image filenames
String arcReactorImages[5];  // Up to 5 different Arc Reactor backgrounds
int numArcImages = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Multi-Mode Digital Clock with JPEG Support");

  // Configure button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize NeoPixel LED ring
  pixels.begin();
  pixels.setBrightness(led_ring_brightness);

  // Turn on all LEDs one by one with color based on current mode
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(
                              modeColors[currentMode].r,
                              modeColors[currentMode].g,
                              modeColors[currentMode].b));
    pixels.show();
    delay(50);
  }

  // Flash the LEDs to show initialization
  flashEffect();

  SPI.end();
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS

  // Initialize TFT display with optimized settings
  Serial.println("Initializing display...");
  tft.init();
  tft.setRotation(0);

  // This helps reduce flickering for some displays
  SPI.setFrequency(27000000);  // Set SPI clock to 27MHz for stability

  tft.fillScreen(PIP_BLACK);

  // Get display dimensions
  screenCenterX = tft.width() / 2;
  screenCenterY = tft.height() / 2;
  screenRadius = min(screenCenterX, screenCenterY) - 10;

  Serial.print("Display dimensions: ");
  Serial.print(tft.width());
  Serial.print("x");
  Serial.println(tft.height());

  // Initialize EEPROM for theme storage
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM Mount Failed");
  } else {
    Serial.println("EEPROM Initialized");
  }

  // Initialize SPIFFS for image storage
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  } else {
    Serial.println("SPIFFS Mounted");
    // List files in SPIFFS for debugging
    listSPIFFSFiles();
  }

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);           // Set the jpeg scale (1, 2, 4, or 8)
  TJpgDec.setSwapBytes(true);       // Set byte swap option if needed for your display
  TJpgDec.setCallback(tft_output);  // Set the callback function for rendering

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  // Wait for connection with timeout
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    // Setup time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Display connection status on screen
    tft.fillScreen(PIP_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(40, 80);
    tft.println("WiFi Connected");
    tft.setCursor(40, 110);
    tft.println("Getting Time...");
    delay(1000);
  } else {
    Serial.println();
    Serial.println("WiFi connection failed. Using default values.");

    // Display status on screen
    tft.fillScreen(PIP_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(40, 100);
    tft.println("WiFi Failed");
    tft.setCursor(30, 130);
    tft.println("Using Default Time");
    delay(1000);
  }

  // Check for available images
  checkForImageFiles();

  // Check for saved settings
  if (EEPROM.readUChar(VALID_FLAG_ADDRESS) == VALID_SETTINGS_FLAG) {
    // We have valid settings
    int savedThemeID = EEPROM.readInt(THEME_ADDRESS);
    int savedMode = EEPROM.readInt(MODE_ADDRESS);

    Serial.print("Found saved settings - Theme ID: ");
    Serial.print(savedThemeID);
    Serial.print(", Mode: ");
    Serial.println(savedMode);

    // Make sure mode is valid
    if (savedMode >= 0 && savedMode < MODE_TOTAL) {
      currentMode = savedMode;

      // Apply the saved theme based on ID
      const char* themePrefix = "ironman";  // Default

      switch (savedThemeID) {
        case THEME_ID_IRONMAN:
          themePrefix = "ironman";
          // Set Iron Man theme colors
          modeColors[0] = { 0, 20, 255, 0x051F };  // Blue digital
          modeColors[1] = { 0, 20, 255, 0x051F };  // Blue analog
          break;
        case THEME_ID_HULK:
          themePrefix = "hulk";
          // Set Hulk theme colors
          modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital
          modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog
          break;
        case THEME_ID_CAPTAIN:
          themePrefix = "captain";
          // Set Captain America theme colors
          modeColors[0] = { 0, 50, 255, 0x037F };  // Blue digital
          modeColors[1] = { 0, 50, 255, 0x037F };  // Blue analog
          break;
        case THEME_ID_THOR:
          themePrefix = "thor";
          // Set Thor theme colors
          modeColors[0] = { 255, 255, 0, 0xFFE0 };  // Yellow digital
          modeColors[1] = { 255, 255, 0, 0xFFE0 };  // Yellow analog
          break;
        case THEME_ID_BLACK_WIDOW:
          themePrefix = "widow";
          // Set Black Widow theme colors
          modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
          modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
          break;
        case THEME_ID_SPIDERMAN:
          themePrefix = "spiderman";
          // Set Spiderman theme colors
          modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
          modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
          break;
      }

      // Apply the theme settings
      Serial.print("Applying saved theme: ");
      Serial.println(themePrefix);

      // Update the LED colors to match the saved theme
      updateLEDs();

      // After loading the saved theme and mode, add this to find a matching image
      if ((currentMode == MODE_ARC_DIGITAL || currentMode == MODE_ARC_ANALOG) && numArcImages > 0) {
        // Get theme prefix based on saved theme ID
        String themePrefix = "ironman";  // Default
        int savedThemeID = EEPROM.readInt(THEME_ADDRESS);

        // Map theme ID to prefix
        switch (savedThemeID) {
          case THEME_ID_IRONMAN: themePrefix = "ironman"; break;
          case THEME_ID_HULK: themePrefix = "hulk"; break;
          case THEME_ID_CAPTAIN: themePrefix = "captain"; break;
          case THEME_ID_THOR: themePrefix = "thor"; break;
          case THEME_ID_BLACK_WIDOW: themePrefix = "widow"; break;
          case THEME_ID_SPIDERMAN: themePrefix = "spiderman"; break;
        }

        Serial.print("Looking for images matching theme: ");
        Serial.println(themePrefix);

        // Check all available images for a match
        themePrefix.toLowerCase();  // Convert to lowercase for comparison
        String matchingImage = "";

        // Debug info - print all available images
        Serial.println("Available images:");
        for (int i = 0; i < numArcImages; i++) {
          Serial.print("  ");
          Serial.println(arcReactorImages[i]);
        }

        // Now look for a matching image
        for (int i = 0; i < numArcImages; i++) {
          String imageName = arcReactorImages[i];
          // Extract just the filename without path
          int lastSlash = imageName.lastIndexOf('/');
          if (lastSlash >= 0) {
            imageName = imageName.substring(lastSlash + 1);
          }

          // Convert to lowercase for case-insensitive comparison
          String lowerImageName = imageName;
          lowerImageName.toLowerCase();

          Serial.print("Checking if ");
          Serial.print(lowerImageName);
          Serial.print(" contains ");
          Serial.println(themePrefix);

          // Check if this image matches our theme
          if (lowerImageName.indexOf(themePrefix) >= 0) {
            matchingImage = arcReactorImages[i];  // Use the full path from our array
            Serial.print("Found matching image: ");
            Serial.println(matchingImage);
            break;
          }
        }

        // Use the matching image or fallback to first available
        if (matchingImage.length() > 0) {
          Serial.print("Displaying theme image: ");
          Serial.println(matchingImage);
          displayJPEG(matchingImage.c_str());
        } else {
          Serial.println("No matching theme image found, using first available");
          displayJPEG(arcReactorImages[0].c_str());
        }
      }
    }
  }

  // Draw initial interface based on current mode
  // This ensures the interface is fully drawn even if we loaded a saved theme
  switchMode(currentMode);
}

// Helper function to list files in SPIFFS
void listSPIFFSFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  Serial.println("Files in SPIFFS:");
  while (file) {
    Serial.print("  ");
    Serial.print(file.name());
    Serial.print(" (");
    Serial.print(file.size());
    Serial.println(" bytes)");
    file = root.openNextFile();
  }
  Serial.println("----------------------");
}

void loop() {
  unsigned long currentMillis = millis();

  // Check for button press (to switch modes)
  checkButtonPress();

  // Get current time if needed
  bool timeUpdateNeeded = (currentMillis - lastTimeCheck >= 1000);
  bool colonUpdateNeeded = (currentMillis - lastColonBlink >= 500);

  // Update based on current mode
  if (currentMode == MODE_ARC_DIGITAL) {
    // Arc Reactor digital mode
    if (timeUpdateNeeded) {
      lastTimeCheck = currentMillis;
      updateTimeAndDate();
      updateDigitalTime();
    }

    // Handle blinking colon for Arc Reactor digital mode
    if (colonUpdateNeeded) {
      lastColonBlink = currentMillis;
      updateArcDigitalColon();
    }
  } else if (currentMode == MODE_ARC_ANALOG) {
    // Arc Reactor analog mode
    if (timeUpdateNeeded) {
      lastTimeCheck = currentMillis;
      updateTimeAndDate();

      // Check if full refresh is needed
      if (needClockRefresh) {
        // Reset the flag first to avoid loops
        needClockRefresh = false;
        // Call the case directly by reusing switchMode with the same mode
        switchMode(MODE_ARC_ANALOG);
      } else {
        // Normal update if no refresh needed
        updateAnalogClock();
      }
    }
  } else if (currentMode == MODE_PIPBOY) {
    // Pip-Boy mode
    if (timeUpdateNeeded) {
      lastTimeCheck = currentMillis;
      updateTimeAndDate();
      updatePipBoyTime();
    }

    // Update the GIF animation
    updatePipBoyGif();
  }

  // LED ring effect every minute (when seconds = 0)
  if (seconds == 0 && (minutes == 0 || minutes == 30) && (currentMillis - lastTimeCheck < 1000)) {
    flashEffect();
  } else {
    // Only update LEDs if needed to avoid SPI conflicts
    static unsigned long lastLEDUpdate = 0;
    if (currentMillis - lastLEDUpdate > 500) {  // Update LEDs at most every 500ms
      updateLEDs();
      lastLEDUpdate = currentMillis;
    }
  }
}

// Improved displayJPEG function
bool displayJPEG(const char* filename) {
  Serial.print("\n--- JPEG Display Attempt ---\n");
  Serial.print("Trying to display: ");
  Serial.println(filename);

  // Always ensure we have a leading slash for SPIFFS
  String filePath = String(filename);
  if (!filePath.startsWith("/")) {
    filePath = "/" + filePath;
  }

  if (!SPIFFS.exists(filePath.c_str())) {
    Serial.print("ERROR: File does not exist: ");
    Serial.println(filePath);
    return false;
  }

  Serial.print("File exists, opening: ");
  Serial.println(filename);

  // Extract just the filename part for theme detection (keep your existing code)
  String fullPath = filePath;
  String justFilename = fullPath;
  int lastSlash = fullPath.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < fullPath.length() - 1) {
    justFilename = fullPath.substring(lastSlash + 1);
  }

  Serial.print("Extracted filename for theme: ");
  Serial.println(justFilename);

  // Set the theme based on the filename
  setThemeFromFilename(justFilename.c_str());

  // Save the current theme selection to EEPROM is now handled in setThemeFromFilename

  File jpegFile = SPIFFS.open(filename, "r");
  if (!jpegFile) {
    Serial.println("ERROR: Failed to open JPEG file despite it existing");
    return false;
  }

  // Get file size
  size_t fileSize = jpegFile.size();
  Serial.print("File size: ");
  Serial.print(fileSize);
  Serial.println(" bytes");

  if (fileSize == 0) {
    Serial.println("ERROR: File is empty (0 bytes)");
    jpegFile.close();
    return false;
  }

  // Use the TJpgDec library to decode and display the JPEG
  uint8_t* jpegBuffer = (uint8_t*)malloc(fileSize);
  if (!jpegBuffer) {
    Serial.println("ERROR: Failed to allocate memory for JPEG");
    jpegFile.close();
    return false;
  }

  size_t bytesRead = jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  Serial.print("Bytes read: ");
  Serial.print(bytesRead);
  Serial.print(" of ");
  Serial.println(fileSize);

  if (bytesRead != fileSize) {
    Serial.println("ERROR: Failed to read entire file");
    free(jpegBuffer);
    return false;
  }

  // Decode and render the JPEG
  Serial.println("Decoding and displaying JPEG...");
  bool success = TJpgDec.drawJpg(0, 0, jpegBuffer, fileSize);

  if (!success) {
    Serial.println("ERROR: TJpgDec.drawJpg failed to decode the image");
  } else {
    Serial.println("JPEG decoded and displayed successfully!");
    saveCurrentThemeAndMode();
  }

  saveCurrentThemeAndMode();

  // Free the buffer
  free(jpegBuffer);
  return success;
}

void checkForImageFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  numArcImages = 0;  // Reset counter

  Serial.println("\n----- Available Background Images -----");

  while (file && numArcImages < 5) {
    String fileName = file.name();

    if (!fileName.startsWith("/")) {
      fileName = "/" + fileName;
    }
    
    // Check if this is a JPEG file
    if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg")) {
      arcReactorImages[numArcImages] = fileName;

      // Extract just the filename without path for display
      String justName = fileName;
      int lastSlash = fileName.lastIndexOf('/');
      if (lastSlash >= 0 && lastSlash < fileName.length() - 1) {
        justName = fileName.substring(lastSlash + 1);
      }
      justName.toLowerCase();

      // Determine and report theme
      String theme = "Default";
      if (justName.startsWith(THEME_IRONMAN)) theme = "Iron Man (Blue/Red)";
      else if (justName.startsWith(THEME_HULK)) theme = "Hulk (Green)";
      else if (justName.startsWith(THEME_CAPTAIN)) theme = "Captain America (Blue/Red)";
      else if (justName.startsWith(THEME_THOR)) theme = "Thor (Cyan/Yellow)";
      else if (justName.startsWith(THEME_BLACK_WIDOW)) theme = "Black Widow (Red)";

      // Log file with theme
      Serial.print(numArcImages + 1);
      Serial.print(": ");
      Serial.print(fileName);
      Serial.print(" - Theme: ");
      Serial.println(theme);

      numArcImages++;
    }

    file = root.openNextFile();
  }

  Serial.println("---------------------------------------");

  if (numArcImages == 0) {
    Serial.println("No JPEG images found in SPIFFS");
  } else {
    Serial.print("Found ");
    Serial.print(numArcImages);
    Serial.println(" background images");
  }
}

void checkButtonPress() {
  // Check if button is pressed (will be LOW because of INPUT_PULLUP)
  if (digitalRead(BUTTON_PIN) == LOW) {
    // Debounce
    if (millis() - lastButtonPress > debounceDelay) {
      // Change mode
      currentMode = (currentMode + 1) % MODE_TOTAL;
      Serial.print("Switching to mode: ");
      Serial.println(currentMode);

      // Switch to the new mode
      switchMode(currentMode);

      saveCurrentThemeAndMode();

      // Flash LEDs to indicate mode change
      flashEffect();

      // Save the current theme and mode
      saveCurrentThemeAndMode();

      lastButtonPress = millis();
    }
  }
}

// Find an image file matching the specified theme prefix
String findImageForTheme(const char* themePrefix) {
  String prefix = String(themePrefix);
  prefix.toLowerCase();

  for (int i = 0; i < numArcImages; i++) {
    String fileName = arcReactorImages[i];
    String baseName = fileName;

    // Extract just the filename without path
    int lastSlash = fileName.lastIndexOf('/');
    if (lastSlash >= 0 && lastSlash < fileName.length() - 1) {
      baseName = fileName.substring(lastSlash + 1);
    }

    baseName.toLowerCase();

    if (baseName.indexOf(prefix) >= 0) {
      return fileName;  // Return the full path
    }
  }

  return "";  // No matching image found
}

String getImagePathForTheme(String themeName) {
  // Convert theme name to lowercase for consistent matching
  themeName.toLowerCase();

  // Check all available images for a match with this theme
  for (int i = 0; i < numArcImages; i++) {
    String imagePath = arcReactorImages[i];
    String justFileName = imagePath;

    // Extract just the filename without path
    int lastSlash = imagePath.lastIndexOf('/');
    if (lastSlash >= 0 && lastSlash < imagePath.length() - 1) {
      justFileName = imagePath.substring(lastSlash + 1);
    }

    // Convert to lowercase
    justFileName.toLowerCase();

    // Check if this image matches the theme
    if (justFileName.startsWith(themeName)) {
      Serial.print("Found matching image for theme: ");
      Serial.print(themeName);
      Serial.print(" -> ");
      Serial.println(imagePath);
      return imagePath;
    }
  }

  // No matching image found
  Serial.print("No matching image found for theme: ");
  Serial.println(themeName);
  return "";
}

void switchMode(int mode) {
  // Store old mode
  int oldMode = currentMode;

  // If switching away from Pip-Boy mode, clean up GIF resources
  if (currentMode == MODE_PIPBOY) {
    cleanupPipBoyMode();
  }

  // Clear screen
  tft.fillScreen(PIP_BLACK);

  // Update current mode
  currentMode = mode;

  // Draw appropriate interface based on mode
  switch (mode) {
    case MODE_ARC_DIGITAL:
      // Arc Reactor digital interface with JPEG background
      if (numArcImages > 0) {
        // Choose the first available Arc Reactor image
        displayJPEG(arcReactorImages[0].c_str());
      } else {
        // Fallback to drawing if no images available
        drawArcReactorBackground();
      }
      // Reset previous values to force redraw
      resetArcDigitalVariables();
      updateDigitalTime();
      break;

    case MODE_ARC_ANALOG:
      // Arc Reactor analog interface with JPEG background
      if (numArcImages > 0) {
        // Use the same or different image (for variety)
        int imageIndex = (numArcImages > 1) ? 1 : 0;
        displayJPEG(arcReactorImages[imageIndex].c_str());
      } else {
        // Fallback to drawing if no images available
        drawArcReactorBackground();
      }
      drawAnalogClock();
      break;

    case MODE_PIPBOY:
      // Pip-Boy interface
      drawPipBoyInterface();
      break;
  }

  // Update LEDs to match the new mode/theme
  updateLEDs();

  // If mode changed, save the setting
  if (oldMode != currentMode) {
    saveCurrentTheme(getCurrentThemeName().c_str(), currentMode);
  }
}

// Save current theme and mode to EEPROM
void saveCurrentThemeAndMode(int themeID) {
  // If no theme ID is provided, try to determine it from colors
  if (themeID == THEME_ID_DEFAULT) {
    // Determine which theme is active based on colors
    if (modeColors[0].r == 0 && modeColors[0].g == 20 && modeColors[0].b == 255) {
      themeID = THEME_ID_IRONMAN;  // Iron Man (blue)
    } else if (modeColors[0].r == 0 && modeColors[0].g == 255 && modeColors[0].b == 50) {
      themeID = THEME_ID_HULK;  // Hulk (green)
    } else if (modeColors[0].r == 0 && modeColors[0].g == 50 && modeColors[0].b == 255) {
      themeID = THEME_ID_CAPTAIN;  // Captain America (blue)
    } else if (modeColors[0].r == 255 && modeColors[0].g == 255 && modeColors[0].b == 0) {
      themeID = THEME_ID_THOR;  // Thor (yellow)
    } else if (modeColors[0].r == 255 && modeColors[0].g == 0 && modeColors[0].b == 0) {
      // Could be Black Widow or Spiderman, both are red
      // Let's just use Black Widow for simplicity
      themeID = THEME_ID_BLACK_WIDOW;
    }
  }

  Serial.print("Saving theme ID: ");
  Serial.print(themeID);
  Serial.print(", Mode: ");
  Serial.println(currentMode);

  // Write the values to EEPROM
  EEPROM.writeInt(THEME_ADDRESS, themeID);
  EEPROM.writeInt(MODE_ADDRESS, currentMode);
  EEPROM.writeUChar(VALID_FLAG_ADDRESS, VALID_SETTINGS_FLAG);

  // Commit the changes
  EEPROM.commit();
}

// Add this function definition to your main .ino file
void setThemeFromFilename(const char* filename) {
  // Create a debug message showing exactly what filename we're checking
  Serial.print("Setting theme from filename: ");
  Serial.println(filename);

  // Create a clean version of the filename (lowercase, no path)
  String fname = String(filename);
  fname.toLowerCase();

  // Remove leading slash if present
  if (fname.startsWith("/")) {
    fname = fname.substring(1);
  }

  // Debug the cleaned filename
  Serial.print("Cleaned filename for theme check: ");
  Serial.println(fname);

  // Default is blue theme for both modes
  // Apply default theme first
  modeColors[0] = { 0, 20, 255, 0x051F };  // Blue digital
  modeColors[1] = { 0, 20, 255, 0x051F };  // Blue analog

  // Let's do more explicit theme debugging
  if (fname.indexOf(THEME_IRONMAN) >= 0) {
    // Iron Man theme - Blue for both
    modeColors[0] = { 0, 20, 255, 0x051F };  // Blue digital
    modeColors[1] = { 0, 20, 255, 0x051F };  // Blue analog (matching digital)
    Serial.println("→ Matched Iron Man theme (blue)");
    saveCurrentThemeAndMode(THEME_ID_IRONMAN);
  } else if (fname.indexOf(THEME_HULK) >= 0) {
    // Hulk theme - Green for both
    modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital
    modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog
    Serial.println("→ Matched Hulk theme (green)");
    saveCurrentThemeAndMode(THEME_ID_HULK);
  } else if (fname.indexOf(THEME_CAPTAIN) >= 0) {
    // Captain America theme - Blue for both
    modeColors[0] = { 0, 50, 255, 0x037F };  // Blue digital
    modeColors[1] = { 0, 50, 255, 0x037F };  // Blue analog (matching digital)
    Serial.println("→ Matched Captain America theme (blue)");
    saveCurrentThemeAndMode(THEME_ID_CAPTAIN);
  } else if (fname.indexOf(THEME_THOR) >= 0) {
    // Thor theme - Yellow for both
    modeColors[0] = { 255, 255, 0, 0xFFE0 };  // Yellow digital
    modeColors[1] = { 255, 255, 0, 0xFFE0 };  // Yellow analog
    Serial.println("→ Matched Thor theme (yellow)");
    saveCurrentThemeAndMode(THEME_ID_THOR);
  } else if (fname.indexOf(THEME_BLACK_WIDOW) >= 0) {
    // Black Widow theme - Red for both
    modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
    modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
    Serial.println("→ Matched Black Widow theme (red)");
    saveCurrentThemeAndMode(THEME_ID_BLACK_WIDOW);
  } else if (fname.indexOf(THEME_SPIDERMAN) >= 0) {
    // Spiderman theme - Red for both
    modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
    modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
    Serial.println("→ Matched Spiderman theme (red)");
    saveCurrentThemeAndMode(THEME_ID_SPIDERMAN);
  } else {
    // No theme matched, use default
    Serial.println("→ No specific theme matched, using default (blue)");
    saveCurrentThemeAndMode(THEME_ID_DEFAULT);
  }

  // Print the selected colors for debugging
  Serial.print("Selected colors - Digital: RGB(");
  Serial.print(modeColors[0].r);
  Serial.print(",");
  Serial.print(modeColors[0].g);
  Serial.print(",");
  Serial.print(modeColors[0].b);
  Serial.print("), Analog: RGB(");
  Serial.print(modeColors[1].r);
  Serial.print(",");
  Serial.print(modeColors[1].g);
  Serial.print(",");
  Serial.print(modeColors[1].b);
  Serial.println(")");

  // Update the LED colors immediately
  updateLEDs();
}

void listAllFilesWithDetails() {
  Serial.println("\n----- COMPLETE SPIFFS FILE LISTING -----");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    String fileName = file.name();
    Serial.print("File: \"");
    Serial.print(fileName);
    Serial.print("\" Size: ");
    Serial.print(file.size());
    Serial.print(" bytes, Exists check: ");
    Serial.println(SPIFFS.exists(fileName) ? "YES" : "NO");

    // Also check with and without slash
    String withoutSlash = fileName;
    if (withoutSlash.startsWith("/")) {
      withoutSlash = withoutSlash.substring(1);
      Serial.print("  Without slash: \"");
      Serial.print(withoutSlash);
      Serial.print("\" Exists check: ");
      Serial.println(SPIFFS.exists(withoutSlash.c_str()) ? "YES" : "NO");
    }

    file = root.openNextFile();
  }
  Serial.println("-----------------------------------------");
}
