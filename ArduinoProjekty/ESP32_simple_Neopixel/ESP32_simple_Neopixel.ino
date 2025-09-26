/*
ESP32 → NeoPixel (WS2812/WS2812B) – 8 LED

ZAPOJENÍ (bezpečně a stabilně):
--------------------------------
• LED: NeoPixel 5 V (piny: +5V, DIN, GND)
• Napájení LED: 5 V zdroj (ne z 3.3 V ESP32!)
• SPOLEČNÁ ZEM: GND zdroje 5 V propojit s GND ESP32

Piny:
• ESP32 GPIO 5  → (přes 330–470 Ω rezistor) → DIN NeoPixel
• ESP32 GND     → GND NeoPixel + GND 5 V zdroje
• 5 V zdroj     → +5V NeoPixel
• Doporučeno: elektrolytický kondenzátor 1000 µF / ≥6.3 V mezi +5V a GND u LED (správná polarita)

Poznámky:
• Většinou funguje 3.3 V datový signál z ESP32 přímo do DIN. Při problémech použij 5 V level shifter (74AHCT125/74HCT14).
• 8 LED na plnou bílou může vzít až ~0.5 A; tento kód drží nízký jas.
• Nainstaluj knihovnu “Adafruit NeoPixel” (Arduino IDE → Library Manager).

Úpravy:
• NUM_LEDS – počet LED (zde 8)
• DATA_PIN – zvolený GPIO (zde 5)
• BRIGHTNESS – jas 0–255 (zde 40)
*/

#include <Adafruit_NeoPixel.h>

#define NUM_LEDS   8
#define DATA_PIN   5
#define BRIGHTNESS 40

Adafruit_NeoPixel strip(NUM_LEDS, DATA_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // všechno zhasnout

  // Rychlý test: červená → zelená → modrá
  colorWipe(strip.Color(255, 0, 0), 60);
  colorWipe(strip.Color(0, 255, 0), 60);
  colorWipe(strip.Color(0, 0, 255), 60);
  colorWipe(strip.Color(0, 0, 0), 20);
}

void loop() {
  rainbowCycle(4);                     // plynulá duha
  theaterChase(strip.Color(0, 40, 120), 70); // “kino” efekt
  breathe(strip.Color(255, 80, 10), 6);      // dýchání
}

/************ Efekty ************/

void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void theaterChase(uint32_t c, uint8_t wait) {
  for (int j = 0; j < 24; j++) {
    for (int q = 0; q < 3; q++) {
      for (uint16_t i = 0; i < strip.numPixels(); i += 3) {
        strip.setPixelColor(i + q, c);
      }
      strip.show();
      delay(wait);
      for (uint16_t i = 0; i < strip.numPixels(); i += 3) {
        strip.setPixelColor(i + q, 0);
      }
    }
  }
}

void breathe(uint32_t c, uint8_t cycles) {
  uint8_t r = (uint8_t)(c >> 16);
  uint8_t g = (uint8_t)(c >> 8);
  uint8_t b = (uint8_t)(c);
  for (uint8_t k = 0; k < cycles; k++) {
    for (uint8_t v = 0; v <= BRIGHTNESS; v++) {
      setAllScaled(r, g, b, v);
      delay(12);
    }
    for (int v = BRIGHTNESS; v >= 0; v--) {
      setAllScaled(r, g, b, v);
      delay(12);
    }
  }
}

void setAllScaled(uint8_t r, uint8_t g, uint8_t b, uint8_t scale) {
  float f = (float)scale / (float)BRIGHTNESS;
  uint8_t rr = (uint8_t)(r * f);
  uint8_t gg = (uint8_t)(g * f);
  uint8_t bb = (uint8_t)(b * f);
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, rr, gg, bb);
  }
  strip.show();
}

void rainbowCycle(uint8_t wait) {
  for (uint16_t j = 0; j < 256 * 3; j++) { // 3 průchody duhy
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85)       return strip.Color(255 - pos * 3, 0, pos * 3);
  else if (pos < 170) { pos -= 85; return strip.Color(0, pos * 3, 255 - pos * 3); }
  else { pos -= 170;  return strip.Color(pos * 3, 255 - pos * 3, 0); }
}
