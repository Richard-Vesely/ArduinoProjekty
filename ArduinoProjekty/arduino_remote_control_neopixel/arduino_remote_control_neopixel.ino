/************************************************************
ZAPOJENÍ (česky pro děti)
- NeoPixel 12 kruh:
   • DIN → D6 (přes 330Ω odpor)
   • 5V → 5V Arduina
   • GND → GND Arduina
- IR přijímač (např. VS1838B):
   • OUT → D2 na Arduinu
   • VCC → 5V
   • GND → GND

OVLÁDÁNÍ:
- IR ovladač tlačítko "Power" → zapne/vypne LED
- Tlačítka 1–5 → přepínají show
************************************************************/

#include <Adafruit_NeoPixel.h>
#include <IRremote.h>

#define PIN_NEOPIXEL 6
#define NUMPIXELS 12
#define PIN_IR 2

Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);

IRrecv irrecv(PIN_IR);
decode_results results;

// Stav
bool powerOn = true;
uint8_t currentShow = 1; // 1..5
uint16_t stepIndex = 0;
unsigned long lastStep = 0;
uint16_t intervalMs = 60;

// Funkce na barvy
uint32_t colorHSV8(uint8_t h, uint8_t s, uint8_t v) {
  uint8_t region = h / 43;
  uint8_t rem = (h - (region * 43)) * 6;
  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * rem) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - rem)) >> 8))) >> 8;

  uint8_t r, g, b;
  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
  return pixels.Color(r, g, b);
}

// Efekty
void showRainbow(uint16_t k) {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint8_t hue = (uint8_t)((i * (256 / NUMPIXELS) + k) & 0xFF);
    pixels.setPixelColor(i, colorHSV8(hue, 255, 255));
  }
}

void showTheaterChase(uint16_t k) {
  uint8_t offset = k % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i % 3 == offset) {
      pixels.setPixelColor(i, pixels.Color(255, 100, 0));
    } else {
      pixels.setPixelColor(i, 0);
    }
  }
}

void showWipe(uint16_t k) {
  uint16_t pos = k % (NUMPIXELS + 1);
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i < pos) pixels.setPixelColor(i, pixels.Color(0, 150, 255));
    else pixels.setPixelColor(i, 0);
  }
}

void showComet(uint16_t k) {
  uint16_t head = k % NUMPIXELS;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint32_t c = pixels.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    r = (r * 180) / 255;
    g = (g * 180) / 255;
    b = (b * 180) / 255;
    pixels.setPixelColor(i, r, g, b);
  }
  pixels.setPixelColor(head, pixels.Color(255, 255, 200));
}

void showSparkle(uint16_t k) {
  (void)k;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint32_t c = pixels.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8) & 0xFF;
    uint8_t b = c & 0xFF;
    r = (r * 200) / 255;
    g = (g * 200) / 255;
    b = (b * 200) / 255;
    pixels.setPixelColor(i, r, g, b);
  }
  uint16_t i = random(NUMPIXELS);
  pixels.setPixelColor(i, pixels.Color(255, 255, 255));
}

// vykreslení podle show
void renderShow() {
  switch (currentShow) {
    case 1: showRainbow(stepIndex); break;
    case 2: showTheaterChase(stepIndex); break;
    case 3: showWipe(stepIndex); break;
    case 4: showComet(stepIndex); break;
    case 5: showSparkle(stepIndex); break;
  }
}

void setup() {
  pixels.begin();
  pixels.clear();
  pixels.show();
  irrecv.enableIRIn();
  randomSeed(analogRead(0));
}

void loop() {
  // IR příjem
  if (irrecv.decode()) {
    unsigned long code = irrecv.decodedIRData.command;
    // vypiš do Serial pro debug
    Serial.println(code);

    switch (code) {
      case 162: // Power
        powerOn = !powerOn;
        break;
      case 48: currentShow = 1; break; // 1
      case 24: currentShow = 2; break; // 2
      case 122: currentShow = 3; break; // 3
      case 16: currentShow = 4; break; // 4
      case 56: currentShow = 5; break; // 5
    }
    irrecv.resume();
  }

  unsigned long now = millis();
  if (now - lastStep >= intervalMs) {
    lastStep = now;
    stepIndex++;
    if (powerOn) {
      renderShow();
      pixels.show();
    } else {
      pixels.clear();
      pixels.show();
    }
  }
}
