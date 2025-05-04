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
#include <SPIFFS.h>
#include <TJpg_Decoder.h>

// Include custom header files for different clock modes
#include "utils.h"
#include "led_controls.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"
#include "theme_persistence.h"

// EEPROM settings
#define EEPROM_SIZE 16            // Size of EEPROM allocation
#define THEME_ADDRESS 0           // Address to store theme ID
#define MODE_ADDRESS 4            // Address to store current mode
#define VALID_FLAG_ADDRESS 8      // Address for valid settings flag
#define VALID_SETTINGS_FLAG 0xAA  // Flag value indicating valid settings

// Theme IDs
#define THEME_ID_DEFAULT 0
#define THEME_ID_IRONMAN 1
#define THEME_ID_HULK 2
#define THEME_ID_CAPTAIN 3
#define THEME_ID_THOR 4
#define THEME_ID_BLACK_WIDOW 5
#define THEME_ID_SPIDERMAN 6
#define THEME_ID_PIPBOY 7  // Add dedicated Pip-Boy theme ID

// Hardware pins
#define LED_PIN 21     // NeoPixel LED ring pin
#define NUMPIXELS 35   // Number of LEDs in the ring
#define BUTTON_PIN 22  // Button pin for mode switching

// Clock modes
#define MODE_ARC_DIGITAL 0  // Arc Reactor digital mode
#define MODE_ARC_ANALOG 1   // Arc Reactor analog mode
#define MODE_PIPBOY 2       // Pip-Boy theme
#define MODE_TOTAL 3        // Total number of modes

// Initialize hardware
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
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

// Mode and timing variables
int currentMode = MODE_ARC_DIGITAL;  // Start with Arc Reactor digital mode
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 300;  // Debounce time in milliseconds

// Array to store available image filenames
String arcReactorImages[5];  // Up to 5 different Arc Reactor backgrounds
int numArcImages = 0;

// Function declarations
void cleanupPipBoyMode();
void updatePipBoyGif();
void checkForImageFiles();
void saveCurrentThemeAndMode(int themeID = THEME_ID_DEFAULT);
void switchMode(int mode);
bool displayJPEG(const char* filename);
void checkButtonPress();
void listSPIFFSFiles();

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

  // Initialize SPI for the display
  SPI.end();
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS

  // Initialize TFT display
  Serial.println("Initializing display...");
  tft.init();
  tft.setRotation(0);
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
    listSPIFFSFiles();
  }

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

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
    Serial.println("\nWiFi connected!");
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
    Serial.println("\nWiFi connection failed. Using default values.");

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

      // Special case: if we're in Pip-Boy mode, set the appropriate theme
      if (savedMode == MODE_PIPBOY) {
        // Force the Pip-Boy theme (green)
        modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital (not used in Pip-Boy but set for consistency)
        modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog (not used in Pip-Boy but set for consistency)
        modeColors[2] = { 0, 255, 50, 0x07E0 };  // Green for Pip-Boy mode
        Serial.println("Applying saved Pip-Boy mode and theme");
      } else {
        // For Arc Reactor modes, apply the saved theme based on ID
        const char* themePrefix = "ironman";  // Default

        switch (savedThemeID) {
          case THEME_ID_IRONMAN:
            themePrefix = "ironman";
            modeColors[0] = { 0, 20, 255, 0x051F };  // Blue digital
            modeColors[1] = { 0, 20, 255, 0x051F };  // Blue analog
            break;
          case THEME_ID_HULK:
            themePrefix = "hulk";
            modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital
            modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog
            break;
          case THEME_ID_CAPTAIN:
            themePrefix = "captain";
            modeColors[0] = { 0, 50, 255, 0x037F };  // Blue digital
            modeColors[1] = { 0, 50, 255, 0x037F };  // Blue analog
            break;
          case THEME_ID_THOR:
            themePrefix = "thor";
            modeColors[0] = { 255, 255, 0, 0xFFE0 };  // Yellow digital
            modeColors[1] = { 255, 255, 0, 0xFFE0 };  // Yellow analog
            break;
          case THEME_ID_BLACK_WIDOW:
            themePrefix = "widow";
            modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
            modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
            break;
          case THEME_ID_SPIDERMAN:
            themePrefix = "spiderman";
            modeColors[0] = { 255, 0, 0, 0xF800 };  // Red digital
            modeColors[1] = { 255, 0, 0, 0xF800 };  // Red analog
            break;
        }

        // Apply the theme settings
        Serial.print("Applying saved theme: ");
        Serial.println(themePrefix);

        // Find matching image for the theme if in Arc Reactor modes
        if ((currentMode == MODE_ARC_DIGITAL || currentMode == MODE_ARC_ANALOG) && numArcImages > 0) {
          // Look for matching image
          String matchingImage = "";

          for (int i = 0; i < numArcImages; i++) {
            String imageName = arcReactorImages[i];
            // Extract filename without path and convert to lowercase
            int lastSlash = imageName.lastIndexOf('/');
            if (lastSlash >= 0) {
              imageName = imageName.substring(lastSlash + 1);
            }
            String lowerImageName = imageName;
            lowerImageName.toLowerCase();

            // Check if image matches theme
            if (lowerImageName.indexOf(themePrefix) >= 0) {
              matchingImage = arcReactorImages[i];
              Serial.print("Found matching image: ");
              Serial.println(matchingImage);
              break;
            }
          }

          // Use matching image or fallback to first available
          if (matchingImage.length() > 0) {
            displayJPEG(matchingImage.c_str());
          } else {
            Serial.println("No matching theme image found, using first available");
            displayJPEG(arcReactorImages[0].c_str());
          }
        }
      }

      // Update the LED colors to match the saved theme/mode
      updateLEDs();
    }
  }

  // Draw initial interface based on current mode
  switchMode(currentMode);
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

// Display JPEG image from SPIFFS
bool displayJPEG(const char* filename) {
  Serial.print("\nDisplaying JPEG: ");
  Serial.println(filename);

  // Ensure we have a leading slash for SPIFFS
  String filePath = String(filename);
  if (!filePath.startsWith("/")) {
    filePath = "/" + filePath;
  }

  if (!SPIFFS.exists(filePath.c_str())) {
    Serial.print("ERROR: File does not exist: ");
    Serial.println(filePath);
    return false;
  }

  // Extract filename for theme detection
  String justFilename = filePath;
  int lastSlash = filePath.lastIndexOf('/');
  if (lastSlash >= 0 && lastSlash < filePath.length() - 1) {
    justFilename = filePath.substring(lastSlash + 1);
  }

  // Set the theme based on the filename
  setThemeFromFilename(justFilename.c_str());

  // Read JPEG file into buffer
  File jpegFile = SPIFFS.open(filePath.c_str(), "r");
  if (!jpegFile) {
    Serial.println("ERROR: Failed to open JPEG file");
    return false;
  }

  size_t fileSize = jpegFile.size();
  if (fileSize == 0) {
    Serial.println("ERROR: File is empty");
    jpegFile.close();
    return false;
  }

  // Allocate buffer and read file
  uint8_t* jpegBuffer = (uint8_t*)malloc(fileSize);
  if (!jpegBuffer) {
    Serial.println("ERROR: Failed to allocate memory");
    jpegFile.close();
    return false;
  }

  size_t bytesRead = jpegFile.read(jpegBuffer, fileSize);
  jpegFile.close();

  if (bytesRead != fileSize) {
    Serial.println("ERROR: Failed to read entire file");
    free(jpegBuffer);
    return false;
  }

  // Decode and display
  bool success = TJpgDec.drawJpg(0, 0, jpegBuffer, fileSize);
  free(jpegBuffer);

  if (!success) {
    Serial.println("ERROR: Failed to decode JPEG");
  } else {
    Serial.println("JPEG displayed successfully");
  }

  return success;
}

// Check for image files in SPIFFS
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

      // Extract filename for display and theme detection
      String justName = fileName;
      int lastSlash = fileName.lastIndexOf('/');
      if (lastSlash >= 0 && lastSlash < fileName.length() - 1) {
        justName = fileName.substring(lastSlash + 1);
      }
      justName.toLowerCase();

      // Determine theme
      String theme = "Default";
      if (justName.startsWith(THEME_IRONMAN)) theme = "Iron Man (Blue/Red)";
      else if (justName.startsWith(THEME_HULK)) theme = "Hulk (Green)";
      else if (justName.startsWith(THEME_CAPTAIN)) theme = "Captain America (Blue/Red)";
      else if (justName.startsWith(THEME_THOR)) theme = "Thor (Cyan/Yellow)";
      else if (justName.startsWith(THEME_BLACK_WIDOW)) theme = "Black Widow (Red)";
      else if (justName.startsWith(THEME_SPIDERMAN)) theme = "Spiderman (Red)";

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

// Check for button press to switch modes
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

      // Flash LEDs to indicate mode change
      flashEffect();

      lastButtonPress = millis();
    }
  }
}

// Switch to a different clock mode
void switchMode(int mode) {
  // Store old mode
  int oldMode = currentMode;

  // If switching away from Pip-Boy mode, clean up GIF resources
  if (oldMode == MODE_PIPBOY) {
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
      initAnalogClock();
      drawAnalogClock();
      break;

    case MODE_PIPBOY:
      // Pip-Boy interface - directly draw the interface without trying to load JPEG
      drawPipBoyInterface();

      // Set theme for Pip-Boy - explicitly set all mode colors for consistency
      modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green digital
      modeColors[1] = { 0, 255, 50, 0x07E0 };  // Green analog
      modeColors[2] = { 0, 255, 50, 0x07E0 };  // Green for Pip-Boy mode

      // Explicitly save the Pip-Boy mode and theme right away to ensure persistence
      EEPROM.writeInt(THEME_ADDRESS, THEME_ID_PIPBOY);
      EEPROM.writeInt(MODE_ADDRESS, MODE_PIPBOY);
      EEPROM.writeUChar(VALID_FLAG_ADDRESS, VALID_SETTINGS_FLAG);
      EEPROM.commit();

      Serial.println("Pip-Boy mode activated and saved to EEPROM");
      break;
  }

  // Update LEDs to match the new mode/theme
  updateLEDs();

  // If mode changed and we're not in Pip-Boy mode (already saved above),
  // save the settings to EEPROM
  if (oldMode != currentMode && currentMode != MODE_PIPBOY) {
    saveCurrentThemeAndMode();
  }
}

// Save current theme and mode to EEPROM
void saveCurrentThemeAndMode(int themeID) {
  // Special handling for Pip-Boy mode
  if (currentMode == MODE_PIPBOY) {
    themeID = THEME_ID_PIPBOY;  // Use dedicated Pip-Boy theme ID

    Serial.print("Saving Pip-Boy mode with theme ID: ");
    Serial.println(themeID);
  }
  // If no theme ID is provided and not in Pip-Boy mode, try to determine it from colors
  else if (themeID == THEME_ID_DEFAULT) {
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
      // Use Black Widow for simplicity
      themeID = THEME_ID_BLACK_WIDOW;
    }
  }

  Serial.print("Saving theme ID: ");
  Serial.print(themeID);
  Serial.print(", Mode: ");
  Serial.println(currentMode);

  // Write values to EEPROM
  EEPROM.writeInt(THEME_ADDRESS, themeID);
  EEPROM.writeInt(MODE_ADDRESS, currentMode);
  EEPROM.writeUChar(VALID_FLAG_ADDRESS, VALID_SETTINGS_FLAG);
  EEPROM.commit();
}
