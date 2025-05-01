/*
 * Pip-Boy 3000 Style Digital Clock with GC9A01 Round Display and NeoPixel LED Ring
 * Based on Fallout's iconic wrist-mounted computer
 * Simplified version with focus on time display
 */

#include <SPI.h>
#include <TFT_eSPI.h>
#include "Adafruit_NeoPixel.h"
#include <WiFi.h>
#include <time.h>
#include <SPIFFS.h>  // For file system access

// Define pins for NeoPixel LED ring
#define LED_PIN 21
#define NUMPIXELS 35

// LED ring brightness settings
int led_ring_brightness = 10;      // Normal brightness (0-255)
int led_ring_brightness_flash = 250; // Flash brightness (0-255)

// LED ring color settings - Pip-Boy green
#define red 0
#define green 255
#define blue 50

// Button pin for mode switching
#define BUTTON_PIN 22

// Colors
#define PIP_GREEN 0x07E0 // Bright green
#define PIP_BLACK 0x0000

// Initialize NeoPixel library
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// TFT Display settings
TFT_eSPI tft = TFT_eSPI();

// WiFi settings - enter your credentials here
const char* ssid = "SSID";     // Enter your WiFi network name
const char* password = "PASSWORD";  // Enter your WiFi password

// Time settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -28800;  // PST offset (-8 hours * 3600 seconds/hour)
const int daylightOffset_sec = 3600; // Daylight saving time adjustment (1 hour)

// Display variables
int screenCenterX;
int screenCenterY;
int screenRadius;

// Time and date variables
int hours = 12, minutes = 0, seconds = 0;
int day = 1, month = 1, year = 2025;
String dayOfWeek = "WEDNESDAY";
bool is24Hour = false; // Use 12-hour format by default

// Timing variables
unsigned long lastTimeCheck = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 300; // Debounce time in milliseconds

// Function prototypes
void drawPipBoyInterface();
void updateTimeAndDate();
void flashEffect();
void greenLight();

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Pip-Boy 3000 Clock");
  
  // Configure button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize NeoPixel LED ring
  pixels.begin();
  pixels.setBrightness(led_ring_brightness);
  
  // Turn on all LEDs one by one with Pip-Boy green color
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
    pixels.show();
    delay(50);
  }
  
  // Flash the LEDs to show initialization
  flashEffect();
  
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
    tft.setTextColor(PIP_GREEN);
    tft.setCursor(40, 80);
    tft.println("VAULT-TEC");
    tft.setCursor(20, 110);
    tft.println("INITIALIZING...");
    delay(1000);
  } else {
    Serial.println();
    Serial.println("WiFi connection failed. Using default values.");
    
    // Display status on screen
    tft.fillScreen(PIP_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(PIP_GREEN);
    tft.setCursor(40, 80);
    tft.println("VAULT-TEC");
    tft.setCursor(40, 110);
    tft.println("OFFLINE MODE");
    delay(1000);
  }
  
  // Draw initial Pip-Boy interface
  drawPipBoyInterface();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check for button press (for future functionality)
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (currentMillis - lastButtonPress > debounceDelay) {
      // Button was pressed - you can add mode-switching code here
      flashEffect(); // Visual feedback for button press
      lastButtonPress = currentMillis;
    }
  }
  
  // Update time and date every second
  if (currentMillis - lastTimeCheck >= 1000) {
    lastTimeCheck = currentMillis;
    updateTimeAndDate();
  }
  
  // Maintain LED ring effect
  static unsigned long lastLEDUpdate = 0;
  if (currentMillis - lastLEDUpdate > 500) { // Update LEDs every 500ms
    greenLight();
    lastLEDUpdate = currentMillis;
  }
}

void drawPipBoyInterface() {
  // Clear the display
  tft.fillScreen(PIP_BLACK);
  
  // Draw day of week at top - Using appropriate font size
  // Moved lower as requested
  tft.setTextSize(2);
  tft.setTextColor(PIP_GREEN);
  int dayWidth = dayOfWeek.length() * 12; // Approximate width for font
  tft.setCursor(screenCenterX - (dayWidth/2), 25); // Moved lower from 10 to 25
  tft.println(dayOfWeek);
  
  // Draw date with larger font as requested
  tft.setTextSize(2); // Increased from 1 to 2
  char dateStr[20];
  sprintf(dateStr, "%02d.%02d.%04d", day, month, year);
  // Calculate width to center text
  int dateWidth = strlen(dateStr) * 12; // Approximate width for font size 2
  tft.setCursor(screenCenterX - (dateWidth/2), 50); // Position below day of week
  tft.println(dateStr);
  
  // Add a static Pip-Boy figure - moved slightly to the right from the far left
  // Just simple face and body
  int figureX = 75; // Adjusted position more toward center
  
  // Head - simple circle with face
  tft.fillCircle(figureX, 100, 15, PIP_GREEN); // Made slightly larger
  
  // Eyes
  tft.fillCircle(figureX - 5, 97, 2, PIP_BLACK);
  tft.fillCircle(figureX + 5, 97, 2, PIP_BLACK);
  
  // Mouth - simple line for neutral/sad expression
  tft.drawFastHLine(figureX - 5, 105, 10, PIP_BLACK);
  
  // Body - simple rectangle
  tft.fillRect(figureX - 10, 115, 20, 30, PIP_GREEN);
  
  // Arms - simple lines
  tft.drawLine(figureX - 10, 120, figureX - 20, 130, PIP_GREEN);
  tft.drawLine(figureX + 10, 120, figureX + 20, 130, PIP_GREEN);
  
  // Legs - simple lines
  tft.drawLine(figureX - 5, 145, figureX - 5, 165, PIP_GREEN);
  tft.drawLine(figureX + 5, 145, figureX + 5, 165, PIP_GREEN);
  
  // Draw time on right side with larger font size
  tft.setTextSize(5); // Increased from 4 to 5
  char timeStr[6];
  
  // Lower the time display slightly
  int timeY = 80;  // Starting Y position for hours
  
  // Hours
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d", displayHours);
  tft.setCursor(120, timeY); // Left-aligned time
  tft.println(timeStr);
  
  // Seconds (next to hours)
  tft.setTextSize(2);
  sprintf(timeStr, "%02d", seconds);
  tft.setCursor(195, timeY + 10); // Right of hours
  tft.println(timeStr);
  
  // Minutes
  tft.setTextSize(5);
  sprintf(timeStr, "%02d", minutes);
  tft.setCursor(120, timeY + 50); // Below hours
  tft.println(timeStr);
  
  // AM/PM indicator (right of minutes)
  tft.setTextSize(2);
  tft.setCursor(195, timeY + 60); // Right of minutes
  tft.println(hours >= 12 ? "PM" : "AM");
  
  // Draw PIP-BOY 3000 and ROBCO INDUSTRIES higher up on the screen
  tft.setTextSize(2); // Increased from 1 to 2 as requested
  
  // Calculate width to center text
  int pipBoyWidth = 11 * 12; // "PIP-BOY 3000" is 11 chars, approx 12 pixels per char at size 2
  int robcoWidth = 8 * 12; // "ROBCO IND"
  
  // Position them higher on the screen
  tft.setCursor(screenCenterX - (pipBoyWidth/2), 180);
  tft.println("PIP-BOY 3000");
  
  tft.setCursor(screenCenterX - (robcoWidth/2), 200);
  tft.println("ROBCO IND");
}

void updateTimeAndDate() {
  // Get current time and date if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeinfo;
    if(getLocalTime(&timeinfo)) {
      hours = timeinfo.tm_hour;
      minutes = timeinfo.tm_min;
      seconds = timeinfo.tm_sec;
      
      day = timeinfo.tm_mday;
      month = timeinfo.tm_mon + 1; // tm_mon is 0-11
      year = timeinfo.tm_year + 1900; // tm_year is years since 1900
      
      // Get day of week
      switch(timeinfo.tm_wday) {
        case 0: dayOfWeek = "SUNDAY"; break;
        case 1: dayOfWeek = "MONDAY"; break;
        case 2: dayOfWeek = "TUESDAY"; break;
        case 3: dayOfWeek = "WEDNESDAY"; break;
        case 4: dayOfWeek = "THURSDAY"; break;
        case 5: dayOfWeek = "FRIDAY"; break;
        case 6: dayOfWeek = "SATURDAY"; break;
      }
    }
  } else {
    // Increment time manually if no WiFi
    seconds++;
    if (seconds >= 60) {
      seconds = 0;
      minutes++;
      if (minutes >= 60) {
        minutes = 0;
        hours++;
        if (hours >= 24) {
          hours = 0;
          // Here we could also increment the date, but keeping it simple
        }
      }
    }
  }
  
  // Update time display
  // Day of week at top
  tft.fillRect(screenCenterX - 70, 25, 140, 20, PIP_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(PIP_GREEN);
  int dayWidth = dayOfWeek.length() * 12; // Approximate width for font size 2
  tft.setCursor(screenCenterX - (dayWidth/2), 25);
  tft.println(dayOfWeek);
  
  // Date
  tft.fillRect(screenCenterX - 70, 50, 140, 20, PIP_BLACK);
  tft.setTextSize(2);
  char dateStr[20];
  sprintf(dateStr, "%02d.%02d.%04d", day, month, year);
  int dateWidth = strlen(dateStr) * 12;
  tft.setCursor(screenCenterX - (dateWidth/2), 50);
  tft.println(dateStr);
  
  // Define time position
  int timeY = 80;
  char timeStr[6];
  
  // Hours - with larger font
  tft.fillRect(120, timeY, 70, 40, PIP_BLACK);
  tft.setTextSize(5);
  int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
  sprintf(timeStr, "%02d", displayHours);
  tft.setCursor(120, timeY);
  tft.println(timeStr);
  
  // Seconds (next to hours)
  tft.fillRect(195, timeY + 10, 30, 20, PIP_BLACK);
  tft.setTextSize(2);
  sprintf(timeStr, "%02d", seconds);
  tft.setCursor(195, timeY + 10);
  tft.println(timeStr);
  
  // Minutes - with larger font
  tft.fillRect(120, timeY + 50, 70, 40, PIP_BLACK);
  tft.setTextSize(5);
  sprintf(timeStr, "%02d", minutes);
  tft.setCursor(120, timeY + 50);
  tft.println(timeStr);
  
  // AM/PM (next to minutes)
  tft.fillRect(195, timeY + 60, 30, 20, PIP_BLACK);
  tft.setTextSize(2);
  tft.setCursor(195, timeY + 60);
  tft.println((is24Hour || hours < 12) ? "AM" : "PM");
}

// LED ring functions
void greenLight() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to Pip-Boy green color
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(red, green, blue));
  }
  pixels.show();
}

void flashEffect() {
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
  greenLight();
}
