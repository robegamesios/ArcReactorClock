/*
 * Arc Reactor Digital Clock with GC9A01 Round Display and NeoPixel LED Ring
 * Digital time display with Arc Reactor aesthetic
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>
#include <time.h>

// Define pins for NeoPixel LED ring
#define LED_PIN 21
#define NUMPIXELS 35

// LED ring brightness settings
int led_ring_brightness = 10;      // Normal brightness (0-255)
int led_ring_brightness_flash = 250; // Flash brightness (0-255)

// LED ring color settings
#define red 00
#define green 20
#define blue 255

// Initialize NeoPixel library
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// TFT Display settings
TFT_eSPI tft = TFT_eSPI();

// WiFi settings - enter your credentials here
const char* ssid = "YOUR-WIFI-SSID";     // Enter your WiFi network name
const char* password = "WIFI-PASSWORD";  // Enter your WiFi password

// Time settings
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -28800;  // PST offset (-8 hours * 3600 seconds/hour)
const int   daylightOffset_sec = 3600; // Daylight saving time adjustment (1 hour)

// Clock variables
int clockCenterX;
int clockCenterY;
int clockRadius;
int hours = 12, minutes = 0, seconds = 0;
int prevHours = -1, prevMinutes = -1, prevSeconds = -1; // Track previous time values
bool prevColonState = false;                          // Track previous colon state
bool is24Hour = false;  // Set to true for 24-hour format
bool showColon = true;  // For blinking colon
char timeStr[20];       // Buffer for formatting time strings
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
uint16_t bgColor = 0x000A;  // Very dark blue background color

// Function prototypes
void drawArcReactorBackground();
void updateDigitalTime();
void blue_light();
void flash_cuckoo();

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Arc Reactor Digital Clock");
  
  // Initialize NeoPixel LED ring
  pixels.begin();
  pixels.setBrightness(led_ring_brightness);
  
  // Turn on all LEDs one by one with blue color
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
    pixels.show();
    delay(50);
  }
  
  // Flash the LEDs to show initialization
  flash_cuckoo();
  
  // CRITICAL FIX: Reinitialize SPI with the correct pins
  // This is necessary for many GC9A01 displays to work with ESP32
  SPI.end();
  SPI.begin(18, 19, 23, 5); // SCK, MISO, MOSI, SS
  
  // Initialize TFT display with optimized settings
  Serial.println("Initializing display...");
  tft.init();
  tft.setRotation(0);
  
  // This helps reduce flickering for some displays
  SPI.setFrequency(27000000); // Set SPI clock to 27MHz for stability
  
  tft.fillScreen(TFT_BLACK);
  
  // Get display dimensions
  clockCenterX = tft.width() / 2;
  clockCenterY = tft.height() / 2;
  clockRadius = min(clockCenterX, clockCenterY) - 10;
  
  Serial.print("Display dimensions: ");
  Serial.print(tft.width());
  Serial.print("x");
  Serial.println(tft.height());
  
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
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_GREEN);
    tft.setCursor(40, 80);
    tft.println("WiFi Connected");
    tft.setCursor(40, 110);
    tft.println("Getting Time...");
    delay(1000);
  } else {
    Serial.println();
    Serial.println("WiFi connection failed. Using default time.");
    
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
  
  // Draw initial clock background
  drawArcReactorBackground();
}

void loop() {
  // Get time measurements for better display timing
  unsigned long currentMillis = millis();
  bool timeUpdateNeeded = (currentMillis - lastTimeCheck >= 100);
  bool colonUpdateNeeded = (currentMillis - lastColonBlink >= 500);
  
  // Update time if needed (100ms interval for time accuracy)
  if (timeUpdateNeeded) {
    lastTimeCheck = currentMillis;
    updateDigitalTime();
  }
  
  // Blink colon every 500ms but only update display if necessary
  if (colonUpdateNeeded) {
    lastColonBlink = currentMillis;
    
    // Toggle colon state
    bool oldColonState = showColon;
    showColon = !showColon;
    
    // Only call update if colon state changed and no time update is imminent
    if (oldColonState != showColon) {
      // Update only the colon part, not the entire time display
      tft.fillRect(clockCenterX - 15, clockCenterY - 25, 25, 45, bgColor);
      if (showColon) {
        tft.setTextSize(4);
        tft.setTextColor(TFT_CYAN);
        tft.setCursor(clockCenterX - 10, clockCenterY - 20);
        tft.print(":");
      }
      
      // Update the state tracking
      prevColonState = showColon;
    }
  }
  
  // LED ring effect every minute (when seconds = 0)
  if (seconds == 0 && (currentMillis - lastTimeCheck < 1000)) {
    flash_cuckoo();
  } else {
    // Only update LEDs if needed to avoid SPI conflicts
    static unsigned long lastLEDUpdate = 0;
    if (currentMillis - lastLEDUpdate > 500) { // Update LEDs at most every 500ms
      blue_light();
      lastLEDUpdate = currentMillis;
    }
  }
}

void drawArcReactorBackground() {
  // Clear the display
  tft.fillScreen(TFT_BLACK);
  
  // Draw outer circle of Arc Reactor
  tft.drawCircle(clockCenterX, clockCenterY, clockRadius, TFT_BLUE);
  tft.drawCircle(clockCenterX, clockCenterY, clockRadius - 1, TFT_BLUE);
  
  // Draw inner circle of Arc Reactor with dark blue background
  tft.fillCircle(clockCenterX, clockCenterY, clockRadius * 0.85, TFT_NAVY);
  tft.drawCircle(clockCenterX, clockCenterY, clockRadius * 0.85, TFT_CYAN);
    
  // Draw inner glowing center of Arc Reactor (slightly darker navy blue)
  tft.fillCircle(clockCenterX, clockCenterY, clockRadius * 0.65, 0x000A); // Very dark blue
  tft.drawCircle(clockCenterX, clockCenterY, clockRadius * 0.65, TFT_CYAN);
  
  // Draw decorative circles for Arc Reactor effect
  tft.drawCircle(clockCenterX, clockCenterY, clockRadius * 0.55, TFT_BLUE);
  tft.drawCircle(clockCenterX, clockCenterY, clockRadius * 0.45, TFT_BLUE);
}

void updateDigitalTime() {
  // Get current time if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)) {
      hours = timeinfo.tm_hour;
      minutes = timeinfo.tm_min;
      seconds = timeinfo.tm_sec;
      
      // Convert to 12-hour format if needed
      if (!is24Hour && hours > 12) {
        hours -= 12;
      }
      if (!is24Hour && hours == 0) {
        hours = 12;
      }
    }
  } else {
    // Increment time manually if no WiFi
    if (millis() - lastTimeCheck >= 1000) {
      seconds++;
      if (seconds >= 60) {
        seconds = 0;
        minutes++;
        if (minutes >= 60) {
          minutes = 0;
          hours++;
          if (!is24Hour && hours > 12) {
            hours = 1;
          }
          if (is24Hour && hours > 23) {
            hours = 0;
          }
        }
      }
    }
  }
  
  // Only update the display if the time or colon state has changed
  if (hours != prevHours || minutes != prevMinutes || seconds != prevSeconds || showColon != prevColonState) {
    // Handle seconds update - MOVED TO TOP for symmetry
    if (seconds != prevSeconds) {
      // Format seconds with leading zero if needed 
      if (seconds < 10) {
        sprintf(timeStr, "0%d", seconds);
      } else {
        sprintf(timeStr, "%d", seconds);
      }
      
      // Clear and redraw seconds area at the top of the display
      tft.fillRect(clockCenterX - 15, clockCenterY - 50, 40, 20, bgColor);
      tft.setTextSize(2);
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(clockCenterX - 10, clockCenterY - 50);
      tft.print(timeStr);
    }

    // Handle hours update
    if (hours != prevHours || (minutes != prevMinutes && hours < 10)) {
      // Format hours with leading zero if needed
      if (hours < 10) {
        sprintf(timeStr, "0%d", hours);
      } else {
        sprintf(timeStr, "%d", hours);
      }
      
      // Clear and redraw hours area only if it changed
      // Adjusted position back to the left a bit
      tft.fillRect(clockCenterX - 63, clockCenterY - 25, 65, 45, bgColor);
      tft.setTextSize(4);
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(clockCenterX - 58, clockCenterY - 20);
      tft.print(timeStr);
    }
    
    // Handle colon update (only if colon state changed)
    if (showColon != prevColonState) {
      tft.fillRect(clockCenterX - 15, clockCenterY - 25, 25, 45, bgColor);
      if (showColon) {
        tft.setTextSize(4);
        tft.setTextColor(TFT_CYAN);
        tft.setCursor(clockCenterX - 10, clockCenterY - 20);
        tft.print(":");
      }
    }
    
    // Handle minutes update
    if (minutes != prevMinutes) {
      // Format minutes with leading zero if needed
      if (minutes < 10) {
        sprintf(timeStr, "0%d", minutes);
      } else {
        sprintf(timeStr, "%d", minutes);
      }
      
      // Clear and redraw minutes area only if it changed
      tft.fillRect(clockCenterX + 10, clockCenterY - 25, 50, 45, bgColor);
      tft.setTextSize(4);
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(clockCenterX + 10, clockCenterY - 20);
      tft.print(timeStr);
    }
    
    // Handle AM/PM indicator (only in 12-hour mode and if hour changed)
    if (!is24Hour && (hours != prevHours || (prevHours == -1))) {
      struct tm timeinfo;
      bool isPM = false;
      
      if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
        isPM = (timeinfo.tm_hour >= 12);
      } else {
        // For manual time, use a simple approximation
        isPM = (millis() / (12 * 60 * 60 * 1000)) % 2;
      }
      
      // Position for AM/PM indicator
      tft.fillRect(clockCenterX - 15, clockCenterY + 35, 40, 20, bgColor);
      tft.setTextSize(2);
      tft.setTextColor(TFT_CYAN);
      if (isPM) {
        tft.setCursor(clockCenterX - 10, clockCenterY + 35);
        tft.println("PM");
      } else {
        tft.setCursor(clockCenterX - 10, clockCenterY + 35);
        tft.println("AM");
      }
    }
    
    // Save current state for next comparison
    prevHours = hours;
    prevMinutes = minutes;
    prevSeconds = seconds;
    prevColonState = showColon;
  }
}

// LED ring functions from the original code
void blue_light() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to blue color
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show();
}

void flash_cuckoo() {
  pixels.setBrightness(led_ring_brightness_flash);
  // Set all pixels to white
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(250, 250, 250));
  }
  pixels.show();
  
  // Fade down brightness
  for (int i=led_ring_brightness_flash; i>10; i--) {
    pixels.setBrightness(i);
    pixels.show();
    delay(7);
  }
  blue_light();
}