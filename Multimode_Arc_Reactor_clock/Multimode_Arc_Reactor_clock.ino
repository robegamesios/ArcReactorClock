/*
 * Combined Digital Clock with Multiple Display Modes and Controls
 * Features:
 * 1. Digital Clock Mode with JPEG background
 * 2. Analog Clock Mode with JPEG background 
 * 3. Pip-Boy 3000 Mode (green)
 * Three control buttons:
 * - Button 1 (GPIO 22): Cycle through backgrounds
 * - Button 2 (GPIO 27): Adjust vertical position
 * - Button 3 (GPIO 25): Change LED color
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>
#include <time.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>
#include "simple_storage.h"
#include "gif_digital.h"

// Project specific header files
#include "config.h"
#include "utils.h"
#include "theme_manager.h"
#include "led_controls.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"
#include "weather_theme.h"

// Hardware pins
#define LED_PIN 21         // NeoPixel LED ring pin
#define NUMPIXELS 35       // Number of LEDs in the ring
#define BG_BUTTON_PIN 22   // Button pin for background cycling
#define POS_BUTTON_PIN 27  // Button pin for vertical position
#define CLR_BUTTON_PIN 25  // Button pin for LED color cycling

// LED ring brightness settings
int led_ring_brightness = 100;        // Normal brightness (0-255)
int led_ring_brightness_flash = 250;  // Flash brightness (0-255)

// Clock display modes
#define MODE_ARC_DIGITAL 0
#define MODE_ARC_ANALOG 1
#define MODE_PIPBOY 2
#define MODE_GIF_DIGITAL 3
#define MODE_WEATHER 4
#define MODE_TOTAL 5

#define MAX_BACKGROUNDS 99 // Set max number of backgrounds

// Current mode variable
int currentMode = MODE_ARC_DIGITAL;  // Start with Arc Reactor digital mode

// Initialize hardware
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();

// Vertical position settings
#define POS_TOP -80
#define POS_CENTER 0
#define POS_BOTTOM 80
#define POS_HIDDEN 999  // Special value to hide the clock

// WiFi settings - enter your credentials here
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Time settings
const char* ntpServer = NTP_SERVER;
const long gmtOffset_sec = GMT_OFFSET_SEC;
const int daylightOffset_sec = DAYLIGHT_OFFSET_SEC;

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

// Settings variables
int currentBgIndex = 0;           // Current background index
int currentVertPos = POS_CENTER;  // Current vertical position
bool isClockHidden = false;       // Clock visibility flag

// Button timing variables
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
unsigned long lastBgButtonPress = 0;
unsigned long lastPosButtonPress = 0;
unsigned long lastClrButtonPress = 0;
unsigned long debounceDelay = 300;  // Debounce time in milliseconds

// Array to store available image filenames
String backgroundImages[MAX_BACKGROUNDS];
int numBgImages = 0;

// Function declarations
void checkForImageFiles();
void cycleBgImage();
void cycleVerticalPosition();
void checkButtonPress();
void drawBackground();
void updateClockDisplay();
void saveSettings();
void loadSettings();
void listSPIFFSFiles();
void switchMode(int mode);

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

// Check for image files in SPIFFS and index them
void checkForImageFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  numBgImages = 0;  // Reset counter

  Serial.println("\n----- Available Background Images -----");

  while (file && numBgImages < MAX_BACKGROUNDS) {
    String fileName = file.name();

    if (!fileName.startsWith("/")) {
      fileName = "/" + fileName;
    }

    // Check if this is a JPEG file or GIF file
    if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg") || fileName.endsWith(".gif")) {
      backgroundImages[numBgImages] = fileName;

      // Log the file info
      Serial.print(numBgImages);
      Serial.print(": ");
      Serial.println(fileName);

      numBgImages++;
    }

    file = root.openNextFile();
  }

  Serial.println("---------------------------------------");

  if (numBgImages == 0) {
    Serial.println("No background images found in SPIFFS");
  } else {
    Serial.print("Found ");
    Serial.print(numBgImages);
    Serial.println(" background images");
  }
}

// Function to prioritize Iron Man image
void prioritizeIronManBackground() {
  // Skip if no images
  if (numBgImages <= 1) return;

  // Search for Iron Man image and move it to index 0
  for (int i = 0; i < numBgImages; i++) {
    String lowerName = backgroundImages[i];
    lowerName.toLowerCase();

    // Check if this is an Iron Man image
    if (lowerName.indexOf("00_ironman") >= 0) {
      // If it's not already at position 0, swap it
      if (i > 0) {
        String temp = backgroundImages[0];
        backgroundImages[0] = backgroundImages[i];
        backgroundImages[i] = temp;

        Serial.println("Iron Man background moved to first position");
      }
      break;
    }
  }

  // Ensure currentBgIndex is 0 for initial display
  currentBgIndex = 0;
}

// Switch to a different clock mode - modified for button control
void switchMode(int mode) {
  // Store old mode
  int oldMode = currentMode;
  Serial.print("Switching from mode ");
  Serial.print(oldMode);
  Serial.print(" to mode ");
  Serial.println(mode);

  // Always clean up ALL GIF resources regardless of the mode we're switching from
  // This ensures both Pip-Boy and GIF Digital resources are properly released
  cleanupPipBoyMode();
  cleanupGifDigitalMode();
  // Clean up weather mode resources if needed
  if (oldMode == MODE_WEATHER) {
    cleanupWeatherMode();
  }

  // Clear screen
  tft.fillScreen(TFT_BLACK);

  // Update current mode
  currentMode = mode;

  // Initialize the weather mode if switching to it
  if (mode == MODE_WEATHER) {
    initWeatherTheme();
  }

  // Draw appropriate interface based on mode
  drawBackground();

  if (!isClockHidden) {
    updateClockDisplay();
  }

  // Update LEDs
  updateLEDs();

  Serial.print("Mode switch complete. Now in mode ");
  Serial.println(currentMode);
}

// Handle background image button press
void cycleBgImage() {
  if (numBgImages <= 0) return;

  // Increment the background index
  currentBgIndex = (currentBgIndex + 1) % numBgImages;

  // Get the file extension for the selected background
  String bgFile = backgroundImages[currentBgIndex];
  String lowerBgFile = bgFile;
  lowerBgFile.toLowerCase();  // Convert to lowercase for case-insensitive comparison

  Serial.println("\n----- CYCLING BACKGROUND -----");
  Serial.print("Selected background: ");
  Serial.println(bgFile);

  // Determine the appropriate mode based on extension and filename
  int newMode = currentMode;  // Default to staying in current mode

  if (bgFile.endsWith(".gif")) {
    Serial.println("GIF file detected");

    // Check if it's vaultboy.gif (for Pip-Boy mode)
    if (lowerBgFile.indexOf("vaultboy") >= 0) {
      Serial.println("This is vaultboy.gif - switching to Pip-Boy mode");
      newMode = MODE_PIPBOY;
    } 
    // Check if it's a weather-related GIF to trigger weather mode
    else if (lowerBgFile.indexOf("weather") >= 0) {
      Serial.println("This is a weather-related GIF - switching to Weather mode");
      newMode = MODE_WEATHER;
    }
    else {
      Serial.println("This is a regular GIF - switching to GIF Digital mode");
      newMode = MODE_GIF_DIGITAL;
    }
  } else if (bgFile.endsWith(".jpg") || bgFile.endsWith(".jpeg")) {
    Serial.println("JPEG file detected");

    // Check if it's a weather-related JPEG to trigger weather mode
    if (lowerBgFile.indexOf("weather") >= 0) {
      Serial.println("This is a weather-related JPEG - switching to Weather mode");
      newMode = MODE_WEATHER;
    }
    // Coming from a special mode, switch to digital
    else if (currentMode == MODE_PIPBOY || currentMode == MODE_GIF_DIGITAL || currentMode == MODE_WEATHER) {
      Serial.println("Coming from a special mode, switching to Arc Digital");
      newMode = MODE_ARC_DIGITAL;
    }
  }

  Serial.print("Current mode: ");
  Serial.print(currentMode);
  Serial.print(", New mode: ");
  Serial.println(newMode);

  // Only perform a mode switch if needed
  if (newMode != currentMode) {
    Serial.println("Mode change required - calling switchMode()");
    switchMode(newMode);
  } else {
    Serial.println("Staying in same mode, just updating background");

    // Clear the screen completely first
    tft.fillScreen(TFT_BLACK);

    // Draw the new background
    drawBackground();

    // Add a small delay to ensure the background is fully rendered
    delay(50);

    // Force a complete refresh of the clock display
    if (!isClockHidden) {
      if (currentMode == MODE_ARC_DIGITAL) {
        // Reset all tracking variables to force complete redraw
        resetArcDigitalVariables();
        // Now redraw the time
        updateDigitalTime();
      } else if (currentMode == MODE_GIF_DIGITAL) {
        // Reset all tracking variables to force complete redraw
        resetGifDigitalVariables();
        // Now redraw the time
        updateGifDigitalTime();
      }
    }
  }

  // Save settings
  saveSettings();
  Serial.println("----- CYCLING COMPLETE -----");
}

// Handle vertical position button press
void cycleVerticalPosition() {
  // Check the current mode first
  if (currentMode == MODE_PIPBOY || currentMode == MODE_WEATHER) {
    // For Pip-Boy mode, just cycle vertical positions without changing mode
    if (currentVertPos == POS_TOP) {
      currentVertPos = POS_CENTER;
    } else if (currentVertPos == POS_CENTER) {
      currentVertPos = POS_BOTTOM;
    } else if (currentVertPos == POS_BOTTOM) {
      currentVertPos = POS_HIDDEN;
      isClockHidden = true;
    } else {
      currentVertPos = POS_TOP;
      isClockHidden = false;
    }
  } else {
    // For both digital modes (arc digital and gif digital), use this sequence:
    // Digital -80 -> Digital 0 -> Digital 80 -> Digital hidden ->
    // Analog visible -> Digital -80 (repeat)

    if (currentMode == MODE_ARC_DIGITAL || currentMode == MODE_GIF_DIGITAL) {
      // We're in digital mode
      if (currentVertPos == POS_TOP) {
        // Move to center position (still digital)
        currentVertPos = POS_CENTER;
      } else if (currentVertPos == POS_CENTER) {
        // Move to bottom position (still digital)
        currentVertPos = POS_BOTTOM;
      } else if (currentVertPos == POS_BOTTOM) {
        // Hide the clock (still digital)
        currentVertPos = POS_HIDDEN;
        isClockHidden = true;
      } else {
        // Switch to analog mode at top position
        currentMode = MODE_ARC_ANALOG;
        currentVertPos = POS_TOP;
        isClockHidden = false;

        // Update settings and switch mode
        CLOCK_VERTICAL_OFFSET = currentVertPos;
        saveSettings();
        switchMode(MODE_ARC_ANALOG);
        return;  // Exit early to avoid double updates
      }
    } else if (currentMode == MODE_ARC_ANALOG) {
      // We're in analog mode - next press goes back to digital
      // Check what kind of background file we have
      String bgFile = backgroundImages[currentBgIndex];
      if (bgFile.endsWith(".gif")) {
        currentMode = MODE_GIF_DIGITAL;  // GIF file uses GIF digital mode
      } else {
        currentMode = MODE_ARC_DIGITAL;  // JPEG file uses Arc digital mode
      }
      currentVertPos = POS_TOP;
      isClockHidden = false;

      // Update settings and switch mode
      CLOCK_VERTICAL_OFFSET = currentVertPos;
      saveSettings();
      switchMode(currentMode);
      return;  // Exit early to avoid double updates
    }
  }

  // Update CLOCK_VERTICAL_OFFSET
  CLOCK_VERTICAL_OFFSET = currentVertPos;

  // Update the display
  drawBackground();
  if (!isClockHidden) {
    updateClockDisplay();
  }

  // Save settings
  saveSettings();
}

// Check for button presses
void checkButtonPress() {
  // Check background button (GPIO 22)
  if (digitalRead(BG_BUTTON_PIN) == LOW) {
    if (millis() - lastBgButtonPress > debounceDelay) {
      cycleBgImage();
      lastBgButtonPress = millis();
    }
  }

  // Check position button (GPIO 27)
  if (digitalRead(POS_BUTTON_PIN) == LOW) {
    if (millis() - lastPosButtonPress > debounceDelay) {
      cycleVerticalPosition();
      lastPosButtonPress = millis();
    }
  }

  // Check color button (GPIO 25)
  if (digitalRead(CLR_BUTTON_PIN) == LOW) {
    if (millis() - lastClrButtonPress > debounceDelay) {
      cycleLedColor();
      updateLEDs();  // Immediately update LEDs to show new color
      saveSettings();
      lastClrButtonPress = millis();
    }
  }

  // Check for force save (holding background and color buttons together)
  if (digitalRead(BG_BUTTON_PIN) == LOW && digitalRead(CLR_BUTTON_PIN) == LOW) {
    static unsigned long forceSaveStartTime = 0;
    static bool forceSaveActive = false;

    if (!forceSaveActive) {
      forceSaveStartTime = millis();
      forceSaveActive = true;
    } else if (millis() - forceSaveStartTime > 2000) {  // Hold for 2 seconds
      // Force save settings
      Serial.println("Force saving settings!");
      saveSettings();

      // Flash LEDs to indicate settings saved
      flashEffect();

      // Reset state
      forceSaveActive = false;

      // Wait until buttons are released
      while (digitalRead(BG_BUTTON_PIN) == LOW || digitalRead(CLR_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  } else {
    // Buttons not held - reset state
    static bool prevForceSaveActive = false;
    if (prevForceSaveActive) {
      prevForceSaveActive = false;
    }
  }
}

// Draw the background based on current settings
void drawBackground() {
  // Clear screen first
  tft.fillScreen(TFT_BLACK);

  if (numBgImages <= 0 || currentBgIndex < 0 || currentBgIndex >= numBgImages) {
    return;  // No valid background to draw
  }

  String bgFile = backgroundImages[currentBgIndex];

  if (currentMode == MODE_PIPBOY) {
    // For Pip-Boy mode, draw the Pip-Boy interface
    drawPipBoyInterface();
  } else if (currentMode == MODE_WEATHER) {
    // For Weather mode, draw the Weather interface
    drawWeatherInterface();
  } else if (currentMode == MODE_GIF_DIGITAL) {
    // For GIF Digital mode, draw the GIF background
    drawGifDigitalBackground(bgFile.c_str());
  } else if (bgFile.endsWith(".jpg") || bgFile.endsWith(".jpeg")) {
    // For JPEG backgrounds in other modes, display the image
    displayJPEGBackground(bgFile.c_str());
  }
}

// Update the clock display based on current settings
void updateClockDisplay() {
  // For GIF Digital mode, always update the GIF animation regardless of clock visibility
  if (currentMode == MODE_GIF_DIGITAL) {
    // This ensures the GIF stays animated even when the clock is hidden
    updateGifDigitalBackground();
  }

  // Skip showing the clock text if hidden
  if (isClockHidden) return;

  switch (currentMode) {
    case MODE_ARC_DIGITAL:
      // Reset digital clock variables
      resetArcDigitalVariables();
      // Update time display
      updateDigitalTime();
      break;

    case MODE_ARC_ANALOG:
      // Initialize and draw analog clock
      initAnalogClock();
      drawAnalogClock();
      break;

    case MODE_PIPBOY:
      // Draw Pip-Boy interface (already done in drawBackground)
      // Just update the time
      updatePipBoyTime();
      break;

    case MODE_GIF_DIGITAL:
      // Reset GIF digital clock variables
      resetGifDigitalVariables();
      // Update time display (but not background - that's handled separately)
      updateGifDigitalTime();
      break;
      
    case MODE_WEATHER:
      // Update the weather display
      updateWeatherTime();
      break;
  }
}

// Save current settings using the simplified approach
void saveSettings() {
  int ledColor = getCurrentLedColor();

  Serial.println("Saving settings to file:");
  Serial.print("  Background Index: ");
  Serial.println(currentBgIndex);
  Serial.print("  Clock Mode: ");
  Serial.println(currentMode);
  Serial.print("  Vertical Position: ");
  Serial.println(currentVertPos);
  Serial.print("  LED Color: ");
  Serial.println(ledColor);

  // Use the simplified file storage
  bool success = saveSettingsToFile(
    currentBgIndex,
    currentMode,
    currentVertPos,
    ledColor);

  if (success) {
    Serial.println("Settings saved successfully");
  } else {
    Serial.println("Failed to save settings");
  }
}

// Load settings using the simplified approach
void loadSettings() {
  int savedBgIndex = 0;
  int savedMode = 0;
  int savedVertPos = 0;
  int savedLedColor = 0;

  bool success = loadSettingsFromFile(
    &savedBgIndex,
    &savedMode,
    &savedVertPos,
    &savedLedColor);

  if (!success) {
    Serial.println("Could not load settings, using defaults");
    return;
  }

  Serial.println("Successfully loaded settings from file");

  // Apply settings if they are in valid range
  if (savedBgIndex >= 0 && savedBgIndex < numBgImages) {
    currentBgIndex = savedBgIndex;
    Serial.print("Using saved background: ");
    Serial.println(currentBgIndex);
  }

  if (savedMode >= 0 && savedMode < MODE_TOTAL) {
    currentMode = savedMode;
    Serial.print("Using saved mode: ");
    Serial.println(currentMode);
  }

  if (savedVertPos == POS_TOP || savedVertPos == POS_CENTER || savedVertPos == POS_BOTTOM || savedVertPos == POS_HIDDEN) {
    currentVertPos = savedVertPos;
    isClockHidden = (savedVertPos == POS_HIDDEN);
    CLOCK_VERTICAL_OFFSET = currentVertPos;
    Serial.print("Using saved position: ");
    Serial.println(currentVertPos);
  }

  if (savedLedColor >= 0 && savedLedColor < COLOR_TOTAL) {
    // Update LED color
    updateModeColorsFromLedColor(savedLedColor);
    Serial.print("Using saved LED color: ");
    Serial.println(savedLedColor);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Multi-Mode Digital Clock with Controls");

  // Configure buttons with pull-up resistors
  pinMode(BG_BUTTON_PIN, INPUT_PULLUP);   // Background cycle button
  pinMode(POS_BUTTON_PIN, INPUT_PULLUP);  // Position cycle button
  pinMode(CLR_BUTTON_PIN, INPUT_PULLUP);  // Color cycle button

  // Initialize NeoPixel LED ring
  pixels.begin();
  pixels.setBrightness(led_ring_brightness);

  // Initialize SPIFFS for image storage and settings storage
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  } else {
    Serial.println("SPIFFS Mounted");
    listSPIFFSFiles();
  }

  // Turn on all LEDs
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(0, 20, 255));  // Default blue
    pixels.show();
    delay(50);
  }

  // Flash the LEDs to show initialization
  flashEffect();  // Use the one from led_controls.h

  // Initialize SPI for the display
  SPI.end();
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS

  // Initialize TFT display
  Serial.println("Initializing display...");
  tft.init();
  tft.setRotation(0);
  SPI.setFrequency(27000000);  // Set SPI clock to 27MHz for stability
  tft.fillScreen(TFT_BLACK);

  // Get display dimensions
  screenCenterX = tft.width() / 2;
  screenCenterY = tft.height() / 2;
  screenRadius = min(screenCenterX, screenCenterY) - 10;

  Serial.print("Display dimensions: ");
  Serial.print(tft.width());
  Serial.print("x");
  Serial.println(tft.height());

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // Test the JPEG decoder with a known good image
  Serial.println("Testing JPEG decoder...");
  if (SPIFFS.exists("/00_ironman.jpg")) {
    File testFile = SPIFFS.open("/00_ironman.jpg", "r");
    if (testFile) {
      size_t testSize = testFile.size();
      if (testSize > 100 && testSize < 150000) {
        uint8_t* testBuf = (uint8_t*)malloc(testSize);
        if (testBuf) {
          testFile.read(testBuf, testSize);
          testFile.close();

          // Clear screen
          tft.fillScreen(TFT_BLACK);

          // Test decoding
          bool testResult = TJpgDec.drawJpg(0, 0, testBuf, testSize);
          free(testBuf);

          Serial.print("JPEG decoder test result: ");
          Serial.println(testResult ? "SUCCESS" : "FAILURE");
        } else {
          testFile.close();
          Serial.println("JPEG test: Failed to allocate memory");
        }
      }
    }
  }

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
    tft.fillScreen(TFT_BLACK);
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
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_RED);
    tft.setCursor(40, 100);
    tft.println("WiFi Failed");
    tft.setCursor(30, 130);
    tft.println("Using Default Time");
    delay(1000);
  }

  // Check for available images before loading settings
  checkForImageFiles();

  prioritizeIronManBackground();

  // Load saved settings - using the file-based method
  loadSettings();

  // Force save settings to ensure they exist for next boot
  // This will create the settings file if it doesn't exist
  saveSettings();

  // Draw the background and clock
  drawBackground();
  if (!isClockHidden) {
    updateClockDisplay();
  }

  // Update LEDs with current color
  updateLEDs();

  Serial.println("Setup complete, entering main loop");
}

void loop() {
  unsigned long currentMillis = millis();

  // Check for button presses
  checkButtonPress();

  // Check if color name overlay needs to be cleared
  checkColorNameTimeout();

  // Check if screen needs refresh from color name timeout
  if (needClockRefresh) {
    needClockRefresh = false;  // Reset the flag
    drawBackground();          // Redraw the background
    if (!isClockHidden) {
      updateClockDisplay();  // Update the clock display
    }
  }

  // Get current time if needed
  bool timeUpdateNeeded = (currentMillis - lastTimeCheck >= 1000);
  bool colonUpdateNeeded = (currentMillis - lastColonBlink >= 500);

  // Skip updates if clock is hidden
  if (!isClockHidden) {
    // Update based on current mode
    if (currentMode == MODE_ARC_DIGITAL) {
      // Digital mode
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();
        updateDigitalTime();
      }

      // Handle blinking colon
      if (colonUpdateNeeded) {
        lastColonBlink = currentMillis;
        updateArcDigitalColon();
      }
    } else if (currentMode == MODE_ARC_ANALOG) {
      // Analog mode
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();

        // Check if full refresh is needed
        if (needClockRefresh) {
          // Reset the flag first to avoid loops
          needClockRefresh = false;
          drawBackground();
          initAnalogClock();
          drawAnalogClock();
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
    } else if (currentMode == MODE_GIF_DIGITAL) {
      // GIF Digital mode
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();
        updateGifDigitalTime();
      }

      // Handle blinking colon
      if (colonUpdateNeeded) {
        lastColonBlink = currentMillis;
        updateGifDigitalColon();
      }

      // Update the GIF animation
      updateGifDigitalBackground();
    } else if (currentMode == MODE_WEATHER) {
      // Weather mode
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();
        updateWeatherTime();
      }

      // Update weather icon if it's animated
      updateWeatherIcon();
      
      // Check if it's time to update weather data (every 10 minutes)
      updateWeatherData();
    }
  } else {
    // Even if clock is hidden, still update GIF animations
    if (currentMode == MODE_GIF_DIGITAL) {
      updateGifDigitalBackground();
    } else if (currentMode == MODE_PIPBOY) {
      updatePipBoyGif();
    }

    // Even if clock is hidden, still update time
    if (timeUpdateNeeded) {
      lastTimeCheck = currentMillis;
      updateTimeAndDate();
    }
  }

  // LED ring effect every minute (when seconds = 0)
  if (seconds == 0 && (minutes == 0 || minutes == 30) && (currentMillis - lastTimeCheck < 1000)) {
    flashEffect();  // Use the one from led_controls.h
  }
}

