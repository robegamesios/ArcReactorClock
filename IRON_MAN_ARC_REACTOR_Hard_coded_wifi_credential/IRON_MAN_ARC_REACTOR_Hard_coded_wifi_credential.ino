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

// Define pins for NeoPixel LED ring
#define LED_PIN 21
#define NUMPIXELS 35

// LED ring brightness settings
int led_ring_brightness = 10;      // Normal brightness (0-255)
int led_ring_brightness_flash = 250; // Flash brightness (0-255)

// Button pin for mode switching
#define BUTTON_PIN 22

// Colors
#define PIP_GREEN 0x07E0 // Bright green for Pip-Boy mode
#define PIP_BLACK 0x0000

// Clock modes
#define MODE_ARC_DIGITAL 0    // Arc Reactor digital mode
#define MODE_ARC_ANALOG 1     // Arc Reactor analog mode
#define MODE_PIPBOY 2         // Pip-Boy theme
#define MODE_TOTAL 3          // Total number of modes

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

// For Arc Reactor mode
int prevHours = -1, prevMinutes = -1, prevSeconds = -1; // Track previous time values
bool prevColonState = false;                           // Track previous colon state
bool showColon = true;                                 // For blinking colon
uint16_t bgColor = 0x000A;                             // Very dark blue background color

// Mode variable
int currentMode = MODE_ARC_DIGITAL;  // Start with Arc Reactor digital mode

// Timing variables
unsigned long lastTimeCheck = 0;
unsigned long lastColonBlink = 0;
unsigned long lastButtonPress = 0;
unsigned long debounceDelay = 300; // Debounce time in milliseconds

// LED color settings for each mode
struct LEDColors {
  int r;
  int g;
  int b;
};

LEDColors modeColors[MODE_TOTAL] = {
  {0, 20, 255},   // Blue for Arc Reactor digital
  {0, 20, 255},   // Blue for Arc Reactor analog
  {0, 255, 50}    // Green for Pip-Boy mode
};

// Function prototypes
void drawPipBoyInterface();
void drawArcReactorBackground();
void updateDigitalTime();
void updatePipBoyTime();
void drawAnalogClock();
void updateTimeAndDate();
void greenLight();
void blueLight();
void flashEffect();
void checkButtonPress();
void switchMode(int mode);
void updateLEDs();

void setup() {
  Serial.begin(115200);
  Serial.println("\nStarting Multi-Mode Digital Clock");
  
  // Configure button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize NeoPixel LED ring
  pixels.begin();
  pixels.setBrightness(led_ring_brightness);
  
  // Turn on all LEDs one by one with color based on current mode
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(
      modeColors[currentMode].r, 
      modeColors[currentMode].g, 
      modeColors[currentMode].b
    ));
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
      
      // Toggle colon state
      bool oldColonState = showColon;
      showColon = !showColon;
      
      // Only call update if colon state changed
      if (oldColonState != showColon) {
        // Update only the colon part, not the entire time display
        tft.fillRect(screenCenterX - 15, screenCenterY - 25, 25, 45, bgColor);
        if (showColon) {
          tft.setTextSize(4);
          tft.setTextColor(TFT_CYAN);
          tft.setCursor(screenCenterX - 10, screenCenterY - 20);
          tft.print(":");
        }
        
        // Update the state tracking
        prevColonState = showColon;
      }
    }
  } 
  else if (currentMode == MODE_ARC_ANALOG) {
    // Arc Reactor analog mode
    if (timeUpdateNeeded) {
      lastTimeCheck = currentMillis;
      updateTimeAndDate();
      
      // Clear center area (keep the background elements)
      tft.fillCircle(screenCenterX, screenCenterY, screenRadius * 0.65, bgColor);
      tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.65, TFT_CYAN);
      
      // Draw analog clock
      drawAnalogClock();
    }
  }
  else if (currentMode == MODE_PIPBOY) {
    // Pip-Boy mode
    if (timeUpdateNeeded) {
      lastTimeCheck = currentMillis;
      updateTimeAndDate();
      updatePipBoyTime();
    }
  }
  
  // LED ring effect every minute (when seconds = 0)
  if (seconds == 0 && (currentMillis - lastTimeCheck < 1000)) {
    flashEffect();
  } else {
    // Only update LEDs if needed to avoid SPI conflicts
    static unsigned long lastLEDUpdate = 0;
    if (currentMillis - lastLEDUpdate > 500) { // Update LEDs at most every 500ms
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
  // Clear screen
  tft.fillScreen(PIP_BLACK);
  
  // Draw appropriate interface based on mode
  switch (mode) {
    case MODE_ARC_DIGITAL:
      // Arc Reactor digital interface
      drawArcReactorBackground();
      // Reset previous values to force redraw
      prevHours = -1;
      prevMinutes = -1;
      prevSeconds = -1;
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
}

void updateLEDs() {
  // Set colors based on current mode
  switch (currentMode) {
    case MODE_ARC_DIGITAL:
    case MODE_ARC_ANALOG:
      blueLight();
      break;
      
    case MODE_PIPBOY:
      greenLight();
      break;
  }
}

// LED ring functions
void greenLight() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to Pip-Boy green color
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(
      modeColors[MODE_PIPBOY].r, 
      modeColors[MODE_PIPBOY].g, 
      modeColors[MODE_PIPBOY].b
    ));
  }
  pixels.show();
}

void blueLight() {
  pixels.setBrightness(led_ring_brightness);
  // Set all pixels to Arc Reactor blue color
  for(int i=0; i<NUMPIXELS; i++) {
    pixels.setPixelColor(i, pixels.Color(
      modeColors[MODE_ARC_DIGITAL].r, 
      modeColors[MODE_ARC_DIGITAL].g, 
      modeColors[MODE_ARC_DIGITAL].b
    ));
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
  
  // Return to appropriate color for current mode
  updateLEDs();
}

//////////////////////////////////////////////
// PIP-BOY MODE FUNCTIONS
//////////////////////////////////////////////

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
  
  // Draw time with larger font size
  tft.setTextSize(5); // Increased size
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
  tft.setTextSize(2);
  
  // Calculate width to center text
  int pipBoyWidth = 11 * 12; // "PIP-BOY 3000" is 11 chars, approx 12 pixels per char at size 2
  int robcoWidth = 8 * 12; // "ROBCO IND" is 8 chars
  
  // Position them higher on the screen
  tft.setCursor(screenCenterX - (pipBoyWidth/2), 180);
  tft.println("PIP-BOY 3000");
  
  tft.setCursor(screenCenterX - (robcoWidth/2), 200);
  tft.println("ROBCO IND");
}

void updatePipBoyTime() {
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

//////////////////////////////////////////////
// ARC REACTOR MODE FUNCTIONS
//////////////////////////////////////////////

void drawArcReactorBackground() {
  // Clear the display
  tft.fillScreen(TFT_BLACK);
  
  // Draw outer circle of Arc Reactor
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius, TFT_BLUE);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius - 1, TFT_BLUE);
  
  // Draw inner circle of Arc Reactor with dark blue background
  tft.fillCircle(screenCenterX, screenCenterY, screenRadius * 0.85, TFT_NAVY);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.85, TFT_CYAN);
    
  // Draw inner glowing center of Arc Reactor (slightly darker navy blue)
  tft.fillCircle(screenCenterX, screenCenterY, screenRadius * 0.65, bgColor); // Very dark blue
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.65, TFT_CYAN);
  
  // Draw decorative circles for Arc Reactor effect
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.55, TFT_BLUE);
  tft.drawCircle(screenCenterX, screenCenterY, screenRadius * 0.45, TFT_BLUE);
}

void updateDigitalTime() {
  // Only update the display if the time or colon state has changed
  if (hours != prevHours || minutes != prevMinutes || seconds != prevSeconds || showColon != prevColonState) {
    // Handle seconds update - at the top for symmetry
    if (seconds != prevSeconds) {
      // Format seconds with leading zero if needed 
      char timeStr[6];
      if (seconds < 10) {
        sprintf(timeStr, "0%d", seconds);
      } else {
        sprintf(timeStr, "%d", seconds);
      }
      
      // Clear and redraw seconds area at the top of the display
      tft.fillRect(screenCenterX - 15, screenCenterY - 50, 40, 20, bgColor);
      tft.setTextSize(2);
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(screenCenterX - 10, screenCenterY - 50);
      tft.print(timeStr);
    }

    // Handle hours update
    if (hours != prevHours || (minutes != prevMinutes && hours < 10)) {
      // Format hours with leading zero if needed
      char timeStr[6];
      int displayHours = is24Hour ? hours : (hours > 12 ? hours - 12 : (hours == 0 ? 12 : hours));
      if (displayHours < 10) {
        sprintf(timeStr, "0%d", displayHours);
      } else {
        sprintf(timeStr, "%d", displayHours);
      }
      
      // Clear and redraw hours area only if it changed
      tft.fillRect(screenCenterX - 63, screenCenterY - 25, 65, 45, bgColor);
      tft.setTextSize(4);
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(screenCenterX - 58, screenCenterY - 20);
      tft.print(timeStr);
    }
    
    // Handle colon update (only if colon state changed)
    if (showColon != prevColonState) {
      tft.fillRect(screenCenterX - 15, screenCenterY - 25, 25, 45, bgColor);
      if (showColon) {
        tft.setTextSize(4);
        tft.setTextColor(TFT_CYAN);
        tft.setCursor(screenCenterX - 10, screenCenterY - 20);
        tft.print(":");
      }
    }
    
    // Handle minutes update
    if (minutes != prevMinutes) {
      // Format minutes with leading zero if needed
      char timeStr[6];
      if (minutes < 10) {
        sprintf(timeStr, "0%d", minutes);
      } else {
        sprintf(timeStr, "%d", minutes);
      }
      
      // Clear and redraw minutes area only if it changed
      tft.fillRect(screenCenterX + 10, screenCenterY - 25, 50, 45, bgColor);
      tft.setTextSize(4);
      tft.setTextColor(TFT_CYAN);
      tft.setCursor(screenCenterX + 10, screenCenterY - 20);
      tft.print(timeStr);
    }
    
    // Handle AM/PM indicator (only in 12-hour mode and if hour changed)
    if (!is24Hour && (hours != prevHours || (prevHours == -1))) {
      struct tm timeinfo;
      bool isPM = false;
      
      if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo)) {
        isPM = (timeinfo.tm_hour >= 12);
      } else {
        // For manual time, determine AM/PM based on hours
        isPM = (hours >= 12);
      }
      
      // Position for AM/PM indicator
      tft.fillRect(screenCenterX - 15, screenCenterY + 35, 40, 20, bgColor);
      tft.setTextSize(2);
      tft.setTextColor(TFT_CYAN);
      if (isPM) {
        tft.setCursor(screenCenterX - 10, screenCenterY + 35);
        tft.println("PM");
      } else {
        tft.setCursor(screenCenterX - 10, screenCenterY + 35);
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

void drawAnalogClock() {
  // Calculate hand angles
  float secondAngle = seconds * 6; // 6 degrees per second
  float minuteAngle = minutes * 6 + (seconds * 0.1); // 6 degrees per minute + slight adjustment for seconds
  float hourAngle = hours * 30 + (minutes * 0.5); // 30 degrees per hour + adjustment for minutes
  
  // Get the hand lengths based on clock radius
  int secondHandLength = screenRadius * 0.8;
  int minuteHandLength = screenRadius * 0.7;
  int hourHandLength = screenRadius * 0.5;
  
  // Draw hour markers (12 dots around the clock)
  for (int i = 0; i < 12; i++) {
    float angle = i * 30 * DEG_TO_RAD; // 30 degrees per hour mark
    int x = screenCenterX + sin(angle) * (screenRadius * 0.8);
    int y = screenCenterY - cos(angle) * (screenRadius * 0.8);
    
    tft.fillCircle(x, y, 3, TFT_CYAN);
  }
  
  // Draw hour hand
  float hourRad = hourAngle * DEG_TO_RAD;
  int hourX = screenCenterX + sin(hourRad) * hourHandLength;
  int hourY = screenCenterY - cos(hourRad) * hourHandLength;
  tft.drawLine(screenCenterX, screenCenterY, hourX, hourY, TFT_WHITE);
  
  // Draw minute hand
  float minuteRad = minuteAngle * DEG_TO_RAD;
  int minuteX = screenCenterX + sin(minuteRad) * minuteHandLength;
  int minuteY = screenCenterY - cos(minuteRad) * minuteHandLength;
  tft.drawLine(screenCenterX, screenCenterY, minuteX, minuteY, TFT_YELLOW);
  
  // Draw second hand
  float secondRad = secondAngle * DEG_TO_RAD;
  int secondX = screenCenterX + sin(secondRad) * secondHandLength;
  int secondY = screenCenterY - cos(secondRad) * secondHandLength;
  tft.drawLine(screenCenterX, screenCenterY, secondX, secondY, TFT_RED);
  
  // Draw center dot
  tft.fillCircle(screenCenterX, screenCenterY, 5, TFT_CYAN);
}
