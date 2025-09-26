/*
==================== ZAPOJENÍ ====================

Arduino Leonardo  ->  NeoPixel 8 (WS2812/WS2812B)
-------------------------------------------------
5V                ->  +5V (VIN) LED
GND               ->  GND LED  (společná zem s Arduinem i PC)
D6                ->  DIN LED  (doporučeno 470 Ω v sérii)
Kondenzátor 1000 µF mezi +5V a GND u LED (správná polarita)

Arduino Leonardo  ->  Rotační enkodér (mechanický)
--------------------------------------------------
GND               ->  GND
D2                ->  CLK / A (INPUT_PULLUP)
D3                ->  DT  / B (INPUT_PULLUP)
D4                ->  SW  (tlačítko; INPUT_PULLUP)

Poznámky:
- Leonardo má D2/D3 vhodné pro rychlé čtení A/B.
- U „modulu“ enkodéru s pinem „+“ připoj + na 5V (napájí jeho pull-up/LED).
- Všechny země musí být společné.
==================================================
PŘÍKAZY po sériové lince (115200 b/s), které posílá stránka:
- "MODE X"  ... X = 0..3  (výběr efektu)
- "BRI N"   ... N = 0..255 (jas)

Události, které posílá Arduino stránce:
- "EV UP", "EV DOWN"  (otáčení)
- "EV PRESS"          (stisk tlačítka)
- "READY"             (po startu)
==================================================
*/

#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

#define LED_PIN    6
#define LED_COUNT  8
#define DEFAULT_BRIGHTNESS 40

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------- ENCODER (bez externí knihovny) ----------
const uint8_t PIN_CLK = 2;  // A / CLK
const uint8_t PIN_DT  = 3;  // B / DT
const uint8_t PIN_SW  = 4;  // tlačítko (na GND)


// How many quadrature steps (edges) make one detent on your encoder.
// Try 4 first; if it still skips every other “click”, try 2.
const int8_t ENC_SCALE = 2;

// Accumulator of quadrature steps since last reported event
volatile int16_t encAccum = 0;
Bounce btn;


// Optional: invert direction if you want the other way around
const bool ENC_INVERT = false;

volatile int8_t encDelta = 0;   // změny z ISR
volatile uint8_t lastAB = 0;

// Čtyřstavová tabulka (Gray) pro dekódování kvadraturního enkodéru
// index = (oldAB<<2) | newAB, hodnota = -1, 0, +1
int8_t qdecTable[16] = {
  0, -1, +1,  0,
  +1, 0,  0, -1,
  -1, 0,  0, +1,
   0, +1, -1, 0
};

inline uint8_t readAB() {
  // Invertujeme, protože INPUT_PULLUP: stisk = LOW -> chceme 1/0 v „normální“ logice
  uint8_t a = !digitalRead(PIN_CLK);
  uint8_t b = !digitalRead(PIN_DT);
  return (a << 1) | b;
}

void updateEncoder() {
  uint8_t newAB = readAB();
  uint8_t idx = (lastAB << 2) | newAB;
  encDelta += qdecTable[idx];
  lastAB = newAB;
}

// ---------- STAV EFEKTŮ ----------
uint8_t currentMode = 0;     // 0..3
uint8_t brightness  = DEFAULT_BRIGHTNESS;
unsigned long tNow;

struct RainbowState { uint16_t j = 0; } rainbow;
struct TheaterState { uint8_t offset = 0; } theater;
struct BounceState  { int pos = 0; int dir = 1; } bounce;
struct SparkleState { } sparkle;

// ---------- HELPERY ----------
uint32_t wheel(byte pos) {
  if (pos < 85) return strip.Color(pos*3, 255 - pos*3, 0);
  if (pos < 170) { pos -= 85; return strip.Color(255 - pos*3, 0, pos*3); }
  pos -= 170; return strip.Color(0, pos*3, 255 - pos*3);
}

// ---------- EFEKTY (neblokující) ----------
void stepRainbow() {
  static unsigned long last=0; if (tNow - last < 25) return; last = tNow;
  for (uint16_t i=0;i<strip.numPixels();i++)
    strip.setPixelColor(i, wheel((i*256/strip.numPixels() + rainbow.j) & 255));
  strip.show();
  rainbow.j = (rainbow.j + 1) & 0xFF;
}
void stepTheater() {
  static unsigned long last=0; if (tNow - last < 70) return; last = tNow;
  strip.clear();
  uint32_t c = strip.Color(255, 60, 0);
  for (uint16_t i = theater.offset; i < strip.numPixels(); i += 3) strip.setPixelColor(i, c);
  strip.show();
  theater.offset = (theater.offset + 1) % 3;
}
void stepBounce() {
  static unsigned long last=0; if (tNow - last < 45) return; last = tNow;
  strip.clear();
  uint32_t cMain = strip.Color(0, 180, 255), cDim = strip.Color(8,8,16);
  strip.setPixelColor(bounce.pos, cMain);
  if (bounce.pos>0) strip.setPixelColor(bounce.pos-1, cDim);
  if (bounce.pos<strip.numPixels()-1) strip.setPixelColor(bounce.pos+1, cDim);
  strip.show();
  bounce.pos += bounce.dir;
  if (bounce.pos<=0 || bounce.pos>=strip.numPixels()-1) bounce.dir = -bounce.dir;
}
void stepSparkle() {
  static unsigned long last=0; if (tNow - last < 30) return; last = tNow;
  for (uint16_t i=0;i<strip.numPixels();i++) strip.setPixelColor(i, strip.Color(0,3,10));
  for (uint8_t s=0;s<2;s++) strip.setPixelColor(random(strip.numPixels()), strip.Color(255,255,255));
  strip.show();
}

// ---------- PŘÍKAZY ZE STRÁNKY ----------
String rx;

void applyMode(uint8_t m) {
  currentMode = m % 4;
  rainbow = RainbowState{};
  theater = TheaterState{};
  bounce  = BounceState{};
  sparkle = SparkleState{};
}
void applyBrightness(uint8_t b) {
  brightness = b; strip.setBrightness(brightness); strip.show();
}
void handleLine(String line) {
  line.trim(); line.toUpperCase();
  if (line.startsWith("MODE")) {
    int sp = line.indexOf(' ');
    if (sp>0) { int m = line.substring(sp+1).toInt(); applyMode((uint8_t)m); }
    Serial.print("OK MODE "); Serial.println(currentMode);
    return;
  }
  if (line.startsWith("BRI")) {
    int sp = line.indexOf(' ');
    if (sp>0) { int v = line.substring(sp+1).toInt(); if(v<0)v=0; if(v>255)v=255; applyBrightness((uint8_t)v); }
    Serial.print("OK BRI "); Serial.println(brightness);
    return;
  }
}

// ---------- SETUP/LOOP ----------
void setup() {
  strip.begin(); strip.setBrightness(brightness); strip.show();

  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);

  btn.attach(PIN_SW); btn.interval(8);

  lastAB = readAB();
  // čteme A i B změny; rychlé, jednoduché
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT),  updateEncoder, CHANGE);

  Serial.begin(115200);
  randomSeed(analogRead(0));
  applyMode(0);
  Serial.println("READY");
}

void loop() {
  tNow = millis();

  // Efekt podle módu
  switch (currentMode) {
    case 0: stepRainbow(); break;
    case 1: stepTheater(); break;
    case 2: stepBounce();  break;
    case 3: stepSparkle(); break;
  }

  // Události z enkodéru → pošli stránce
  // Pull the raw delta from ISR
  noInterrupts();
  int8_t d = encDelta;
  encDelta = 0;
  interrupts();

  // Accumulate raw quadrature steps
  if (d != 0) {
    if (ENC_INVERT) d = -d;
    encAccum += d;

    // Emit one event per full detent (ENC_SCALE steps)
    while (encAccum >= ENC_SCALE) {
      Serial.println("EV DOWN");   // or UP, depending on your preference
      encAccum -= ENC_SCALE;
    }
    while (encAccum <= -ENC_SCALE) {
      Serial.println("EV UP");
      encAccum += ENC_SCALE;
    }
  }


  btn.update();
  if (btn.fell()) Serial.println("EV PRESS");

  // Čti příkazy ze stránky
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch=='\n' || ch=='\r') { if (rx.length()) { handleLine(rx); rx=""; } }
    else { rx += ch; if (rx.length()>64) rx=""; }
  }
}
