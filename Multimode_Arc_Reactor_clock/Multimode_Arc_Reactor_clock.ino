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
#include <EEPROM.h>

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
#define MODE_TOTAL 3

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

// EEPROM storage addresses
#define EEPROM_SIZE 32
#define BG_INDEX_ADDR 0     // Background image index
#define CLOCK_MODE_ADDR 4   // Clock display mode
#define VERT_POS_ADDR 8     // Vertical position
#define LED_COLOR_ADDR 12   // LED color ID
#define VALID_FLAG_ADDR 16  // Valid settings flag
#define VALID_SETTINGS_FLAG 0xAA

// LED color definitions
#define COLOR_BLUE 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_CYAN 4
#define COLOR_PURPLE 5
#define COLOR_WHITE 6
#define COLOR_TOTAL 7

// LED color structure
struct LEDColor {
  int r;
  int g;
  int b;
  uint16_t tft_color;  // TFT color value for display
};

// Define the LED colors array
LEDColor ledColors[COLOR_TOTAL] = {
  { 0, 20, 255, 0x051F },    // Blue
  { 255, 0, 0, 0xF800 },     // Red
  { 0, 255, 50, 0x07E0 },    // Green
  { 255, 255, 0, 0xFFE0 },   // Yellow
  { 0, 255, 255, 0x07FF },   // Cyan
  { 180, 0, 255, 0xC01F },   // Purple
  { 255, 255, 255, 0xFFFF }  // White
};

// For now, include these header files later after defining variables
// they need access to
#include "utils.h"
#include "theme_manager.h"
#include "led_controls.h"
#include "arc_digital.h"
#include "arc_analog.h"
#include "pipboy.h"

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

// Settings variables
int currentBgIndex = 0;            // Current background index
int currentVertPos = POS_CENTER;   // Current vertical position
int currentLedColor = COLOR_BLUE;  // Current LED color
bool isClockHidden = false;        // Clock visibility flag

// Button timing variables
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
unsigned long lastBgButtonPress = 0;
unsigned long lastPosButtonPress = 0;
unsigned long lastClrButtonPress = 0;
unsigned long debounceDelay = 300;  // Debounce time in milliseconds

// Array to store available image filenames
String backgroundImages[10];  // Up to 10 different background images
int numBgImages = 0;

// Function declarations
void checkForImageFiles();
void cycleBgImage();
void cycleVerticalPosition();
void cycleLedColor();
void checkButtonPress();
void drawBackground();
void updateClockDisplay();
void saveSettings();
void loadSettings();
void listSPIFFSFiles();
void updateLEDsWithCurrentColor();
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

  while (file && numBgImages < 10) {
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

// Switch to a different clock mode - modified for button control
void switchMode(int mode) {
  // Store old mode
  int oldMode = currentMode;

  // If switching away from Pip-Boy mode, clean up GIF resources
  if (oldMode == MODE_PIPBOY) {
    cleanupPipBoyMode();
  }

  // Clear screen
  tft.fillScreen(TFT_BLACK);

  // Update current mode
  currentMode = mode;

  // Draw appropriate interface based on mode
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

  // Determine the appropriate mode based on extension
  if (bgFile.endsWith(".gif")) {
    // Switch to Pip-Boy mode for any GIF file
    currentMode = MODE_PIPBOY;
  } else if (currentMode == MODE_PIPBOY) {
    // Coming from Pip-Boy mode, switch to digital
    currentMode = MODE_ARC_DIGITAL;
  }

  // Update the display with the new background
  switchMode(currentMode);

  // Save settings
  saveSettings();
}

// Handle vertical position button press
void cycleVerticalPosition() {
  // Check the current mode first
  if (currentMode == MODE_PIPBOY) {
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
    // For both digital and analog modes, use this sequence:
    // Digital -80 -> Digital 0 -> Digital 80 -> Digital hidden ->
    // Analog visible -> Digital -80 (repeat)

    if (currentMode == MODE_ARC_DIGITAL) {
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
      currentMode = MODE_ARC_DIGITAL;
      currentVertPos = POS_TOP;
      isClockHidden = false;

      // Update settings and switch mode
      CLOCK_VERTICAL_OFFSET = currentVertPos;
      saveSettings();
      switchMode(MODE_ARC_DIGITAL);
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

// Handle LED color button press
// Handle LED color button press
void cycleLedColor() {
  // Cycle through predefined colors
  currentLedColor = (currentLedColor + 1) % COLOR_TOTAL;

  // Update the theme colors based on selected color
  switch (currentLedColor) {
    case COLOR_BLUE:
      modeColors[0] = { 0, 20, 255, 0x051F };  // Blue
      modeColors[1] = { 0, 20, 255, 0x051F };
      break;
    case COLOR_RED:
      modeColors[0] = { 255, 0, 0, 0xF800 };  // Red
      modeColors[1] = { 255, 0, 0, 0xF800 };
      break;
    case COLOR_GREEN:
      modeColors[0] = { 0, 255, 50, 0x07E0 };  // Green
      modeColors[1] = { 0, 255, 50, 0x07E0 };
      break;
    case COLOR_YELLOW:
      modeColors[0] = { 255, 255, 0, 0xFFE0 };  // Yellow
      modeColors[1] = { 255, 255, 0, 0xFFE0 };
      break;
    case COLOR_CYAN:
      modeColors[0] = { 0, 255, 255, 0x07FF };  // Cyan
      modeColors[1] = { 0, 255, 255, 0x07FF };
      break;
    case COLOR_PURPLE:
      modeColors[0] = { 180, 0, 255, 0xC01F };  // Purple
      modeColors[1] = { 180, 0, 255, 0xC01F };
      break;
    case COLOR_WHITE:
      modeColors[0] = { 255, 255, 255, 0xFFFF };  // White
      modeColors[1] = { 255, 255, 255, 0xFFFF };
      break;
  }

  // Always keep Pip-Boy in green
  modeColors[2] = { 0, 255, 50, 0x07E0 };

  // Update LED ring with the new color
  updateLEDs();  // Use the function from led_controls.h

  // If in analog mode, update the seconds ring
  if (currentMode == MODE_ARC_ANALOG && !isClockHidden) {  // Changed from MODE_ANALOG
    needClockRefresh = true;
  }

  // Save settings
  saveSettings();
}

// Update LEDs with current color - wrapper around existing function
void updateLEDsWithCurrentColor() {
  // The updateLEDs function is already defined in led_controls.h and uses modeColors
  updateLEDs();
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
      lastClrButtonPress = millis();
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
  } else if (bgFile.endsWith(".jpg") || bgFile.endsWith(".jpeg")) {
    // For JPEG backgrounds, display the image
    displayJPEGBackground(bgFile.c_str());
  }
}

// Update the clock display based on current settings
void updateClockDisplay() {
  if (isClockHidden) return;  // Don't show the clock if hidden

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
  }
}

// Save current settings to EEPROM
void saveSettings() {
  EEPROM.writeInt(BG_INDEX_ADDR, currentBgIndex);
  EEPROM.writeInt(CLOCK_MODE_ADDR, currentMode);
  EEPROM.writeInt(VERT_POS_ADDR, currentVertPos);
  EEPROM.writeInt(LED_COLOR_ADDR, currentLedColor);
  EEPROM.writeUChar(VALID_FLAG_ADDR, VALID_SETTINGS_FLAG);
  EEPROM.commit();

  Serial.println("Settings saved to EEPROM");
}

// Load settings from EEPROM
void loadSettings() {
  if (EEPROM.readUChar(VALID_FLAG_ADDR) != VALID_SETTINGS_FLAG) {
    Serial.println("No valid settings found in EEPROM");
    return;
  }

  // Read saved values
  int savedBgIndex = EEPROM.readInt(BG_INDEX_ADDR);
  int savedMode = EEPROM.readInt(CLOCK_MODE_ADDR);
  int savedVertPos = EEPROM.readInt(VERT_POS_ADDR);
  int savedLedColor = EEPROM.readInt(LED_COLOR_ADDR);

  // Validate settings
  if (savedBgIndex >= 0 && savedBgIndex < numBgImages) {
    currentBgIndex = savedBgIndex;
  }

  if (savedMode >= 0 && savedMode < MODE_TOTAL) {
    currentMode = savedMode;
  }

  if (savedVertPos == POS_TOP || savedVertPos == POS_CENTER || savedVertPos == POS_BOTTOM || savedVertPos == POS_HIDDEN) {
    currentVertPos = savedVertPos;
    isClockHidden = (savedVertPos == POS_HIDDEN);

    // Update CLOCK_VERTICAL_OFFSET
    CLOCK_VERTICAL_OFFSET = currentVertPos;
  }

  if (savedLedColor >= 0 && savedLedColor < COLOR_TOTAL) {
    currentLedColor = savedLedColor;
    // Apply the saved color
    cycleLedColor();
  }

  Serial.println("Settings loaded from EEPROM");
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

  // Initialize theme system
  initThemeSystem();

  // Turn on all LEDs
  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(
                              modeColors[0].r, modeColors[0].g, modeColors[0].b));
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

  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("EEPROM initialization failed");
  } else {
    Serial.println("EEPROM initialized");
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

  // Check for available images
  checkForImageFiles();

  // Load saved settings
  loadSettings();

  // Draw the background and clock
  drawBackground();
  if (!isClockHidden) {
    updateClockDisplay();
  }

  // Update LEDs with current color
  updateLEDsWithCurrentColor();
}

void loop() {
  unsigned long currentMillis = millis();

  // Check for button presses
  checkButtonPress();

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
    }
  } else {
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
