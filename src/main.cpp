#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- USER CONFIGURATION ---
// LOGICAL SPEED SETTINGS:
// 0.1  = Snail (Very Slow)
// 1.0  = Walking
// 10.0 = Running
// 100.0 = Instant/Blur
#define WALK_SPEED  10

#define SCREEN_TIMEOUT 10000  
#define BUTTON_PIN 3          

// --- PINS ---
#define SDA_PIN 8
#define SCL_PIN 9
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
Adafruit_BME280 bme; 

// --- VARIABLES ---
unsigned long lastButtonPress = 0;
unsigned long lastWalkUpdate = 0;
bool isStatsMode = true;

// We calculate the actual delay in milliseconds based on your speed setting
// We use 100.0 as a base constant to scale it nicely.
const unsigned long stepDelay = (unsigned long)(100.0 / WALK_SPEED);

// Walker Coordinates
int walkerX = SCREEN_WIDTH / 2;
int walkerY = SCREEN_HEIGHT / 2;

// --- MATH HELPER ---
double calculateDewPoint(double tempC, double humidity) {
  double a = 17.27;
  double b = 237.7;
  double alpha = ((a * tempC) / (b + tempC)) + log(humidity / 100.0);
  return (b * alpha) / (a - alpha);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  
  if (!bme.begin(0x76, &Wire)) { 
    if (!bme.begin(0x77, &Wire)) while (1); 
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  lastButtonPress = millis(); 
}

void loop() {
  // --- 1. BUTTON INPUT ---
  if (digitalRead(BUTTON_PIN) == LOW) {
    lastButtonPress = millis();
    if (!isStatsMode) {
      isStatsMode = true; 
    }
  }

  // --- 2. TRANSITION LOGIC ---
  if (isStatsMode && (millis() - lastButtonPress > SCREEN_TIMEOUT)) {
    isStatsMode = false; 
    display.clearDisplay();
    display.display(); 
    walkerX = SCREEN_WIDTH / 2;
    walkerY = SCREEN_HEIGHT / 2;
  }

  // --- 3. MODE: STATS ---
  if (isStatsMode) {
    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F; 
    float spread = temp - calculateDewPoint(temp, hum);

    display.clearDisplay(); 
    
    display.drawLine(0, 14, 128, 14, WHITE);
    display.setCursor(40, 4); display.setTextSize(1); display.print("AIRFISH");

    display.setCursor(4, 25); display.setTextSize(2); 
    display.print(temp, 1); display.setTextSize(1); display.print("C");

    display.setCursor(4, 50);
    display.print("Hum:"); display.print(hum, 0); display.print("%");
    display.setCursor(64, 50);
    display.print("Pre:"); display.print(pres, 0);

    int boxX = 70; int boxY = 18; int boxW = 58; int boxH = 26;
    if (spread <= 3.0) {
      display.fillRect(boxX, boxY, boxW, boxH, WHITE);
      display.setTextColor(BLACK); 
      display.setCursor(boxX + 4, boxY + 5);
      display.setTextSize(1);
      display.println("WET ROCK");
      display.setCursor(boxX + 4, boxY + 15);
      display.print("Dp:"); display.print(spread, 1);
    } else {
      display.drawRect(boxX, boxY, boxW, boxH, WHITE);
      display.setTextColor(WHITE);
      display.setCursor(boxX + 4, boxY + 5);
      display.setTextSize(1);
      display.println("GOOD");
      display.setCursor(boxX + 4, boxY + 15);
      display.print("Dp:"); display.print(spread, 1);
    }
    
    display.setTextColor(WHITE); 
    display.display();
  }

  // --- 4. MODE: WALKER (Now using Logical Speed) ---
else {
    if (millis() - lastWalkUpdate > stepDelay) {
      lastWalkUpdate = millis();

      // 1. Pick a random direction (-1, 0, or 1)
      int stepX = random(-1, 2);
      int stepY = random(-1, 2);

      // 2. Check X Bounce
      // If we are at the edge AND trying to move out, flip the step!
      if ((walkerX <= 0 && stepX == -1) || (walkerX >= SCREEN_WIDTH - 1 && stepX == 1)) {
        stepX = -stepX; // Bounce!
      }

      // 3. Check Y Bounce
      if ((walkerY <= 0 && stepY == -1) || (walkerY >= SCREEN_HEIGHT - 1 && stepY == 1)) {
        stepY = -stepY; // Bounce!
      }

      // 4. Move
      walkerX += stepX;
      walkerY += stepY;

      // (Optional) Keep constrain just as a safety net, though the bounce logic handles it
      walkerX = constrain(walkerX, 0, SCREEN_WIDTH - 1);
      walkerY = constrain(walkerY, 0, SCREEN_HEIGHT - 1);

      display.drawPixel(walkerX, walkerY, WHITE); 
      display.display();
    }
  }
}