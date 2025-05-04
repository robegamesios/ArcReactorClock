/*
 * Combined Digital Clock with Multiple Themes and JPEG Support
 * Includes:
 * 1. Arc Reactor Digital Mode with JPEG background
 * 2. Arc Reactor Analog Mode with JPEG background 
 * 3. Pip-Boy 3000 Mode (green)
 * Button press cycles through the clock faces
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>
#include <time.h>
#include <SPIFFS.h>
#include <TJpg_Decoder.h>

// Include the unified theme manager first (replaces led_controls.h and theme_persistence.h)
#include "theme_manager.h"

// Include custom header files for different clock modes
#include "utils.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"

// Hardware pins
#define LED_PIN 21     // NeoPixel LED ring pin
#define NUMPIXELS 35   // Number of LEDs in the ring
#define BUTTON_PIN 22  // Button pin for mode switching

// LED ring brightness settings (moved from led_controls.h)
int led_ring_brightness = 100;        // Normal brightness (0-255)
int led_ring_brightness_flash = 250;  // Flash brightness (0-255)

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
void checkForImageFiles();
void switchMode(int mode);
void checkButtonPress();
void listSPIFFSFiles();

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
        displayJPEGBackground(arcReactorImages[0].c_str());
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
        displayJPEGBackground(arcReactorImages[imageIndex].c_str());
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

      // Set theme for Pip-Boy mode
      setThemeFromId(THEME_ID_PIPBOY);
      break;
  }

  // Update LEDs to match the new mode/theme
  updateLEDs();

  // Save the current theme and mode to EEPROM
  saveThemeAndMode();
}

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

  // Initialize theme system (replaces EEPROM initialization)
  initThemeSystem();

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

  // Check for saved settings using our new unified system
  int loadedMode, loadedThemeId;
  if (loadSavedThemeAndMode(&loadedMode, &loadedThemeId)) {
    // We have valid settings
    Serial.print("Found saved settings - Theme ID: ");
    Serial.print(loadedThemeId);
    Serial.print(", Mode: ");
    Serial.println(loadedMode);

    // Apply the saved mode
    currentMode = loadedMode;

    // Apply the saved theme
    setThemeFromId(loadedThemeId);

    // Update the LED colors to match the saved theme/mode
    updateLEDs();
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
