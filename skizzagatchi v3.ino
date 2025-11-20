#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define W 64
#define H 32
Adafruit_SSD1306 display(W, H, &Wire, -1);

// ================================
// BUTTON
// ================================
#define BUTTON_PIN 3
bool lastButton = false;
int screenState = 0;

unsigned long pressStart = 0;

// ================================
// MOODS
// ================================
enum Mood { HAPPY, HUNGRY, SAD, SLEEPY };
Mood mood = HAPPY;

unsigned long hungryTimer = 0;
unsigned long sadTimer = 0;

const unsigned long HUNGRY_TIME = 600000;  // 10 minutes
const unsigned long SAD_TIME = 300000;     // 5 minutes

// ================================
// FROG BITMAPS
// ================================
const uint8_t frogNormal[] = {
  0x00,0x00,0x00,0x00,0x0C,0x30,0x1E,0x78,
  0x1E,0x78,0x0F,0xF0,0x08,0x10,0x14,0x50,
  0x10,0x10,0x3B,0xC8,0x10,0x10,0x08,0x30,
  0x07,0xF0,0x0E,0x10,0x00,0x00,0x00,0x00
};

const uint8_t frogBlink[] = {
  0x00,0x00,0x00,0x00,0x0C,0x30,0x1E,0x78,
  0x1E,0x78,0x0F,0xF0,0x00,0x00,0x14,0x50,
  0x00,0x00,0x3B,0xC8,0x10,0x10,0x08,0x30,
  0x07,0xF0,0x0E,0x10,0x00,0x00,0x00,0x00
};

const uint8_t frogSleep[] = {
  0x00,0x00,0x3C,0x00,0x7E,0x00,0xFF,0x00,
  0x81,0x80,0xA5,0x80,0x81,0x80,0x93,0xC0,
  0x81,0x80,0xC0,0x80,0x78,0x00,0x1C,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

const uint8_t pillowBitmap[] = {
  0x3F,0xC0,0x21,0x40,0x21,0x40,
  0x21,0x40,0x21,0x40,0x3F,0xC0
};

const uint8_t zBitmap[] = {
  0x1F,0x02,0x04,0x08,0x10,0x1F,0x00
};

// ================================
// TIMERS / XP
// ================================
unsigned long lastBlink = 0;
bool blinkState = false;

unsigned long lastSleepStart = 0;
bool isSleeping = false;

unsigned long lastUpdate = 0;

unsigned long awakeAccum = 0;
const unsigned long SLEEP_TIME = 180000;     // 3 minutes
const unsigned long SLEEP_DURATION = 30000;  // 30 sec

int level = 1;
unsigned long xpAccum = 0;
const unsigned long XP_PER_LEVEL = 600000;   // 10 min

// ================================
// DRAW FUNCTIONS
// ================================
void drawAwake() {
  if (millis() - lastBlink > 2500) {
    blinkState = true;
    lastBlink = millis();
  }

  if (blinkState && millis() - lastBlink > 150)
    blinkState = false;

  const uint8_t* bmp = blinkState ? frogBlink : frogNormal;
  display.drawBitmap(24, 8, bmp, 16, 16, SSD1306_WHITE);

  // Draw mood label
  display.setTextSize(1);
  display.setCursor(0,0);

  switch (mood) {
    case HAPPY: display.print("HAPPY"); break;
    case HUNGRY: display.print("HUNGRY"); break;
    case SAD: display.print("SAD"); break;
    case SLEEPY: display.print("SLEEPY"); break;
  }
}

void drawSleeping() {
  display.drawBitmap(20, 14, pillowBitmap, 12, 6, SSD1306_WHITE);
  display.drawBitmap(26, 8, frogSleep, 16, 16, SSD1306_WHITE);

  display.drawBitmap(55, 2, zBitmap, 5, 7, SSD1306_WHITE);
  display.drawBitmap(55, 10, zBitmap, 5, 7, SSD1306_WHITE);
}

void drawStatsPage() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.print("LV ");
  display.print(level);

  display.setCursor(0,10);
  display.print("MIN ");
  display.print(awakeAccum / 60000);
}

// ================================
// SETUP
// ================================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  randomSeed(analogRead(0));

  hungryTimer = millis();
  sadTimer = millis();

  lastUpdate = millis();
}

// ================================
// LOOP
// ================================
void loop() {
  unsigned long now = millis();
  unsigned long dt = now - lastUpdate;
  lastUpdate = now;

  // BUTTON
  bool pressed = (digitalRead(BUTTON_PIN) == LOW);

  if (pressed && !lastButton)
    pressStart = now;

  // short press -> switch pages
  if (!pressed && lastButton) {
    if (now - pressStart < 600) {
      screenState = !screenState;
    }
  }

  // long press → fix mood
  if (pressed && (now - pressStart > 2000)) {
    if (mood == HUNGRY || mood == SAD) {
      mood = HAPPY;
      hungryTimer = now;
      sadTimer = now;
    } else if (mood == SLEEPY) {
      isSleeping = true;
      lastSleepStart = now;
    }
  }

  lastButton = pressed;

  // MOOD TIMERS
  if (mood == HAPPY) {
    if (now - hungryTimer > HUNGRY_TIME)
      mood = HUNGRY;
    else if (now - sadTimer > SAD_TIME)
      mood = SAD;
  }

  if (!isSleeping)
    xpAccum += dt;

  if (xpAccum >= XP_PER_LEVEL) {
    xpAccum = 0;
    level++;
  }

  if (!isSleeping)
    awakeAccum += dt;

  if (!isSleeping && awakeAccum >= SLEEP_TIME) {
    mood = SLEEPY;
  }

  if (isSleeping && now - lastSleepStart > SLEEP_DURATION) {
    isSleeping = false;
    awakeAccum = 0;
    mood = HAPPY;
    hungryTimer = now;
    sadTimer = now;
  }

  // DRAW
  display.clearDisplay();

  if (screenState == 0) {
    if (isSleeping) drawSleeping();
    else drawAwake();
  } else {
    drawStatsPage();
  }

  display.display();
}  what ever this says and i already picked my options why are we doing this again
