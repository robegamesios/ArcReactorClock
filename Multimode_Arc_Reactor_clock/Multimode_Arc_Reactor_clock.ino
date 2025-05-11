/*
 * ESP32 Multi-Mode Digital Clock with TFT Display
 * Features:
 * 1. Digital Clock Mode with JPEG background
 * 2. Analog Clock Mode with JPEG background 
 * 3. Pip-Boy 3000 Mode (Fallout-inspired)
 * 4. GIF Digital Mode with animated backgrounds
 * 5. Weather Display Mode with OpenWeatherMap integration
 * 
 * Controls:
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
#include "weather_data.h"
#include "utils.h"
#include "theme_manager.h"
#include "led_controls.h"
#include "weather_led.h"
#include "weather_theme.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"

// Hardware pins
#define LED_PIN 21         // NeoPixel LED ring pin
#define NUMPIXELS 35       // Number of LEDs in the ring
#define BG_BUTTON_PIN 22   // Button pin for background cycling
#define POS_BUTTON_PIN 27  // Button pin for vertical position
#define CLR_BUTTON_PIN 25  // Button pin for LED color cycling

// LED ring brightness settings
int led_ring_brightness = 100;        // Normal brightness (0-255)
int led_ring_brightness_flash = 250;  // Flash brightness (0-255)

// Vertical position settings
#define POS_TOP -80
#define POS_CENTER 0
#define POS_BOTTOM 80
#define POS_HIDDEN 999  // Special value to hide the clock

// Vertical position variable - referenced in various header files
int CLOCK_VERTICAL_OFFSET = 0;

// WiFi settings from config.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Time settings from config.h
const char* ntpServer = NTP_SERVER;
const long gmtOffset_sec = GMT_OFFSET_SEC;
const int daylightOffset_sec = DAYLIGHT_OFFSET_SEC;

// Weather variables
const char* weatherApiKey = WEATHER_API_KEY;
const long weatherCityId = WEATHER_CITY_ID;
char weatherUnits[10] = WEATHER_UNITS;
unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 10 * 60 * 1000;  // 10 minutes

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

// Background image handling
#define MAX_BACKGROUNDS 99
String backgroundImages[MAX_BACKGROUNDS];
int numBgImages = 0;

// Initialize hardware
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
TFT_eSPI tft = TFT_eSPI();

// Weather data initialization
WeatherData currentWeather = { "", "", 0, 0, 0, 0, 0, 0, 0, false };

// Current mode variable
int currentMode = MODE_ARC_DIGITAL;  // Start with Arc Reactor digital mode

// Function declarations
void checkForImageFiles();
void cycleBgImage();
void cycleVerticalPosition();
void checkButtonPress();
void drawBackground();
void updateClockDisplay();
void saveSettings();
void loadSettings();
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
}

// Check for image files in SPIFFS and index them
void checkForImageFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  numBgImages = 0;  // Reset counter

  while (file && numBgImages < MAX_BACKGROUNDS) {
    String fileName = file.name();

    if (!fileName.startsWith("/")) {
      fileName = "/" + fileName;
    }

    // Check if this is a JPEG file or GIF file
    if (fileName.endsWith(".jpg") || fileName.endsWith(".jpeg") || fileName.endsWith(".gif")) {
      backgroundImages[numBgImages] = fileName;
      numBgImages++;
    }

    file = root.openNextFile();
  }

  if (numBgImages == 0) {
    Serial.println("No background images found in SPIFFS");
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
      }
      break;
    }
  }

  // Set to first background
  currentBgIndex = 0;
}

// Switch to a different clock mode
void switchMode(int mode) {
  // Store old mode
  int oldMode = currentMode;

  // Clean up resources from previous mode
  cleanupPipBoyMode();
  cleanupGifDigitalMode();
  if (oldMode == MODE_WEATHER) {
    cleanupWeatherMode();
  }

  // Clear screen
  tft.fillScreen(TFT_BLACK);

  // Update current mode
  currentMode = mode;

  // Initialize weather mode if needed
  if (mode == MODE_WEATHER) {
    initWeatherTheme();
    
    // Set LED color based on temperature if weather data is valid
    if (currentWeather.valid) {
      setWeatherLEDColorDirectly();
    }
  }

  // Draw interface for new mode
  drawBackground();

  if (!isClockHidden) {
    updateClockDisplay();
  }

  // Update LEDs
  updateLEDs();
}

// Handle background image button press
void cycleBgImage() {
  if (numBgImages <= 0) return;

  // Increment the background index
  currentBgIndex = (currentBgIndex + 1) % numBgImages;

  // Get the file extension for the selected background
  String bgFile = backgroundImages[currentBgIndex];
  String lowerBgFile = bgFile;
  lowerBgFile.toLowerCase();  // Convert to lowercase for comparison

  // Determine the appropriate mode based on extension and filename
  int newMode = currentMode;  // Default to staying in current mode

  if (bgFile.endsWith(".gif")) {
    // Check if it's vaultboy.gif (Pip-Boy mode)
    if (lowerBgFile.indexOf("vaultboy") >= 0) {
      newMode = MODE_PIPBOY;
    }
    // Check if it's a weather-related GIF 
    else if (lowerBgFile.indexOf("weather") >= 0) {
      newMode = MODE_WEATHER;
    } else {
      newMode = MODE_GIF_DIGITAL;
    }
  } else if (bgFile.endsWith(".jpg") || bgFile.endsWith(".jpeg")) {
    // Check if it's a weather-related JPEG
    if (lowerBgFile.indexOf("weather") >= 0) {
      newMode = MODE_WEATHER;
    }
    // Coming from a special mode, switch to digital
    else if (currentMode == MODE_PIPBOY || currentMode == MODE_GIF_DIGITAL || currentMode == MODE_WEATHER) {
      newMode = MODE_ARC_DIGITAL;
    }
  }

  // Only perform a mode switch if needed
  if (newMode != currentMode) {
    switchMode(newMode);
  } else {
    // Clear the screen completely first
    tft.fillScreen(TFT_BLACK);

    // Draw the new background
    drawBackground();

    // Force a complete refresh of the clock display
    if (!isClockHidden) {
      if (currentMode == MODE_ARC_DIGITAL) {
        resetArcDigitalVariables();
        updateDigitalTime();
      } else if (currentMode == MODE_GIF_DIGITAL) {
        resetGifDigitalVariables();
        updateGifDigitalTime();
      }
    }
  }

  // Save settings
  saveSettings();
}

// Handle vertical position button press
void cycleVerticalPosition() {
  // Check the current mode first
  if (currentMode == MODE_PIPBOY || currentMode == MODE_WEATHER) {
    // For Pip-Boy and Weather modes, just cycle vertical positions
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
    // For digital modes, cycle through positions including analog mode
    if (currentMode == MODE_ARC_DIGITAL || currentMode == MODE_GIF_DIGITAL) {
      if (currentVertPos == POS_TOP) {
        currentVertPos = POS_CENTER;
      } else if (currentVertPos == POS_CENTER) {
        currentVertPos = POS_BOTTOM;
      } else if (currentVertPos == POS_BOTTOM) {
        currentVertPos = POS_HIDDEN;
        isClockHidden = true;
      } else {
        // Switch to analog mode
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
      // Switch back to digital mode based on background type
      String bgFile = backgroundImages[currentBgIndex];
      currentMode = bgFile.endsWith(".gif") ? MODE_GIF_DIGITAL : MODE_ARC_DIGITAL;
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

// Check for button presses with improved color handling for weather mode
void checkButtonPress() {
  static bool useWeatherColors = true;
  static unsigned long forceSaveStartTime = 0;
  static bool forceSaveActive = false;

  // Check background button (GPIO 22)
  if (digitalRead(BG_BUTTON_PIN) == LOW) {
    if (millis() - lastBgButtonPress > debounceDelay) {
      cycleBgImage();
      lastBgButtonPress = millis();

      // Reset to weather-based colors when changing backgrounds in weather mode
      if (currentMode == MODE_WEATHER) {
        useWeatherColors = true;
      }
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
      // Special handling for weather mode
      if (currentMode == MODE_WEATHER) {
        // Toggle between automatic and manual color modes
        useWeatherColors = !useWeatherColors;

        if (useWeatherColors && currentWeather.valid) {
          updateWeatherLEDs();
        } else {
          cycleLedColor();  // Cycle to next color in manual mode
        }
      } else {
        // Normal behavior for other modes
        cycleLedColor();
      }

      updateLEDs();  // Update LEDs to show new color
      saveSettings();
      lastClrButtonPress = millis();
    }
  }

  // Check for force save (holding background and color buttons together)
  if (digitalRead(BG_BUTTON_PIN) == LOW && digitalRead(CLR_BUTTON_PIN) == LOW) {
    if (!forceSaveActive) {
      forceSaveStartTime = millis();
      forceSaveActive = true;
    } else if (millis() - forceSaveStartTime > 2000) {  // Hold for 2 seconds
      // Force save settings
      saveSettings();

      // Flash LEDs to confirm
      flashEffect();

      // Reset state
      forceSaveActive = false;

      // Wait until buttons are released
      while (digitalRead(BG_BUTTON_PIN) == LOW || digitalRead(CLR_BUTTON_PIN) == LOW) {
        delay(10);
      }
    }
  } else {
    // Reset force save state when buttons released
    forceSaveActive = false;
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
    // Draw the Pip-Boy interface
    drawPipBoyInterface();
  } else if (currentMode == MODE_WEATHER) {
    // Draw the Weather interface
    drawWeatherInterface();
  } else if (currentMode == MODE_GIF_DIGITAL) {
    // Draw the GIF background
    drawGifDigitalBackground(bgFile.c_str());
  } else if (bgFile.endsWith(".jpg") || bgFile.endsWith(".jpeg")) {
    // Display JPEG background in other modes
    displayJPEGBackground(bgFile.c_str());
  }
}

// Update the clock display based on current settings
void updateClockDisplay() {
  // For GIF Digital mode, always update animation regardless of clock visibility
  if (currentMode == MODE_GIF_DIGITAL) {
    updateGifDigitalBackground();
  }

  // Skip showing the clock text if hidden
  if (isClockHidden) return;

  switch (currentMode) {
    case MODE_ARC_DIGITAL:
      resetArcDigitalVariables();
      updateDigitalTime();
      break;

    case MODE_ARC_ANALOG:
      initAnalogClock();
      drawAnalogClock();
      break;

    case MODE_PIPBOY:
      updatePipBoyTime();
      break;

    case MODE_GIF_DIGITAL:
      resetGifDigitalVariables();
      updateGifDigitalTime();
      break;

    case MODE_WEATHER:
      updateWeatherTime();
      break;
  }
}

// Save current settings
void saveSettings() {
  int ledColor = getCurrentLedColor();
  
  // Use file storage for settings
  saveSettingsToFile(
    currentBgIndex,
    currentMode,
    currentVertPos,
    ledColor);
}

// Load settings from file
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
    return;  // Use defaults if settings can't be loaded
  }

  // Apply settings if they are in valid range
  if (savedBgIndex >= 0 && savedBgIndex < numBgImages) {
    currentBgIndex = savedBgIndex;
  }

  if (savedMode >= 0 && savedMode < MODE_TOTAL) {
    currentMode = savedMode;
  }

  if (savedVertPos == POS_TOP || savedVertPos == POS_CENTER || 
      savedVertPos == POS_BOTTOM || savedVertPos == POS_HIDDEN) {
    currentVertPos = savedVertPos;
    isClockHidden = (savedVertPos == POS_HIDDEN);
    CLOCK_VERTICAL_OFFSET = currentVertPos;
  }

  if (savedLedColor >= 0 && savedLedColor < COLOR_TOTAL) {
    updateModeColorsFromLedColor(savedLedColor);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Multi-Mode Digital Clock");

  // Configure buttons with pull-up resistors
  pinMode(BG_BUTTON_PIN, INPUT_PULLUP);   // Background cycle button
  pinMode(POS_BUTTON_PIN, INPUT_PULLUP);  // Position cycle button
  pinMode(CLR_BUTTON_PIN, INPUT_PULLUP);  // Color cycle button

  // Initialize NeoPixel LED ring
  pixels.begin();
  pixels.setBrightness(led_ring_brightness);

  currentWeather.valid = false;  // Mark as invalid until first fetch

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
  flashEffect();

  // Initialize SPI for the display
  SPI.end();
  SPI.begin(18, 19, 23, 5);  // SCK, MISO, MOSI, SS

  // Initialize TFT display
  tft.init();
  tft.setRotation(0);
  SPI.setFrequency(27000000);  // Set SPI clock to 27MHz for stability
  tft.fillScreen(TFT_BLACK);

  // Get display dimensions
  screenCenterX = tft.width() / 2;
  screenCenterY = tft.height() / 2;
  screenRadius = min(screenCenterX, screenCenterY) - 10;

  // Initialize TJpg_Decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);

  // Test JPEG decoder with default image
  if (SPIFFS.exists("/00_ironman.jpg")) {
    File testFile = SPIFFS.open("/00_ironman.jpg", "r");
    if (testFile) {
      size_t testSize = testFile.size();
      if (testSize > 100 && testSize < 150000) {
        uint8_t* testBuf = (uint8_t*)malloc(testSize);
        if (testBuf) {
          testFile.read(testBuf, testSize);
          testFile.close();
          
          tft.fillScreen(TFT_BLACK);
          TJpgDec.drawJpg(0, 0, testBuf, testSize);
          free(testBuf);
        } else {
          testFile.close();
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
    
    // Setup time synchronization
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Display connection status
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(40, 80);
    tft.println("WiFi Connected");
    tft.setCursor(40, 110);
    tft.println("Getting Time...");
    delay(1000);
  } else {
    Serial.println("\nWiFi connection failed");

    // Display status
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

  // Load saved settings
  loadSettings();

  // Ensure settings file exists for next boot
  saveSettings();

  // Draw the background and clock
  drawBackground();
  if (!isClockHidden) {
    updateClockDisplay();
  }

  // Update LEDs with current color
  updateLEDs();
}

void loop() {
  unsigned long currentMillis = millis();

  // Check for button presses
  checkButtonPress();

  // Check if color name overlay needs to be cleared
  checkColorNameTimeout();

  // Check if screen needs refresh from color name timeout
  if (needClockRefresh) {
    needClockRefresh = false;
    drawBackground();
    if (!isClockHidden) {
      updateClockDisplay();
    }
  }

  // Track weather color update timing
  static unsigned long lastWeatherColorCheck = 0;
  if (currentMode == MODE_WEATHER && currentMillis - lastWeatherColorCheck >= 60000) {  // Every minute
    lastWeatherColorCheck = currentMillis;
    if (currentWeather.valid) {
      setWeatherLEDColorDirectly();
    }
  }

  // Time update flags
  bool timeUpdateNeeded = (currentMillis - lastTimeCheck >= 1000);
  bool colonUpdateNeeded = (currentMillis - lastColonBlink >= 500);

  // Skip display updates if clock is hidden
  if (!isClockHidden) {
    // Update based on current mode
    if (currentMode == MODE_ARC_DIGITAL) {
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
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();

        // Check if full refresh is needed
        if (needClockRefresh) {
          needClockRefresh = false;
          drawBackground();
          initAnalogClock();
          drawAnalogClock();
        } else {
          updateAnalogClock();
        }
      }
    } else if (currentMode == MODE_PIPBOY) {
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();
        updatePipBoyTime();
      }

      // Update the GIF animation
      updatePipBoyGif();
    } else if (currentMode == MODE_GIF_DIGITAL) {
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
      if (timeUpdateNeeded) {
        lastTimeCheck = currentMillis;
        updateTimeAndDate();
        updateWeatherTime();
      }

      // Update weather icon if needed
      updateWeatherIcon();

      // Check if it's time to update weather data
      updateWeatherData();
    }
  } else {
    // Even if clock is hidden, still update animations
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

  // LED ring effect every hour or half-hour
  if (seconds == 0 && (minutes == 0 || minutes == 30) && (currentMillis - lastTimeCheck < 1000)) {
    flashEffect();
  }
}
