/*
 * Combined Digital Clock with Multiple Themes
 * Includes:
 * 1. Arc Reactor Digital Mode (blue)
 * 2. Arc Reactor Analog Mode (blue)
 * 3. Pip-Boy 3000 Mode (green)
 * Button press cycles through the clock faces
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>
#include <time.h>
#include <SPIFFS.h>  // For file system access

// Include custom header files for different clock modes
#include "utils.h"
#include "led_controls.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"

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
const char* ssid = "SSID";  // Enter your WiFi network name
const char* password = "PASSWORD";        // Enter your WiFi password

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

// Mode variable
int currentMode = MODE_ARC_DIGITAL;  // Start with Arc Reactor digital mode

// Timing variables
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 300;  // Debounce time in milliseconds

void cleanupPipBoyMode();
void updatePipBoyGif();

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Multi-Mode Digital Clock");

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

  // CRITICAL FIX: Reinitialize SPI with the correct pins
  // This is necessary for many GC9A01 displays to work with ESP32
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

  // Initialize SPIFFS for possible future use
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
  } else {
    Serial.println("SPIFFS Mounted");
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
      updateAnalogClock();
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
  if (seconds == 0 && (currentMillis - lastTimeCheck < 1000)) {
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

void switchMode(int mode) {
  // If switching away from Pip-Boy mode, clean up GIF resources
  if (currentMode == MODE_PIPBOY) {
    cleanupPipBoyMode();
  }

  // Clear screen
  tft.fillScreen(PIP_BLACK);

  // Draw appropriate interface based on mode
  switch (mode) {
    case MODE_ARC_DIGITAL:
      // Arc Reactor digital interface
      drawArcReactorBackground();
      // Reset previous values to force redraw
      resetArcDigitalVariables();
      updateDigitalTime();
      break;

    case MODE_ARC_ANALOG:
      // Arc Reactor analog interface
      drawArcReactorBackground();
      drawAnalogClock();
      break;

    case MODE_PIPBOY:
      // Pip-Boy interface
      drawPipBoyInterface();
      break;
  }
}
