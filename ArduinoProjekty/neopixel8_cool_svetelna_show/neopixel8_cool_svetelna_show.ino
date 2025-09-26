/* 
==================== ZAPOJENÍ (Arduino Leonardo + NeoPixel 8) ====================

Nejčastější modul: WS2812 / WS2812B (NeoPixel), 5 V napájení, 1 datová linka (DIN).

Arduino Leonardo  ->  NeoPixel pásek/modul (8 diod)
--------------------------------------------------
5V                 ->  +5V (VIN) na LED (doporučeno napájet z externího 5 V zdroje,
                        pokud může spotřeba přesáhnout USB; 60 mA max na 1 LED při plné bílé)
GND                ->  GND (společná zem s LED i externím zdrojem)
D6                 ->  DIN (datový vstup na LED)

Doporučení pro spolehlivost:
- 470 Ω rezistor v sérii na datové lince (mezi D6 a DIN).
- Elektrolytický kondenzátor 1000 µF / 6.3 V (nebo vyšší) mezi +5 V a GND u LED (správná polarita!).
- Délka datového vodiče co nejkratší. Společná zem je nutná.
- Leonardo má 5V logiku, takže není potřeba level-shifter.

==================================================================================
*/

#include <Adafruit_NeoPixel.h>

// ====== Nastavení ======
#define LED_PIN     6      // datový pin do DIN
#define LED_COUNT   8      // počet LED na modulu/pásku
#define BRIGHTNESS  40     // 0–255 (rozumné hodnoty 20–80 pro hraní z USB)

// Vytvoření objektu pro NeoPixel (GRB je standard pro WS2812/WS2812B)
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Prototypy efektů
void rainbow(uint8_t wait);
void theaterChase(uint32_t c, uint8_t wait);
void colorWipe(uint32_t c, uint8_t wait);
void bounce(uint32_t c, uint8_t wait);
void sparkle(uint32_t baseColor, uint8_t wait);
uint32_t wheel(byte pos);

void setup() {
  strip.begin();
  strip.setBrightness(BRIGHTNESS);
  strip.show(); // vypnout všechny LED na startu
}

void loop() {
  // 1) Barevný přechod "duha"
  rainbow(10);

  // 2) Divadelní „chase“ se třemi barvami
  theaterChase(strip.Color(255, 0, 0), 60);   // červená
  theaterChase(strip.Color(0, 255, 0), 60);   // zelená
  theaterChase(strip.Color(0, 0, 255), 60);   // modrá

  // 3) Plnění barvami
  colorWipe(strip.Color(255, 80, 0), 25);     // oranžová
  colorWipe(strip.Color(0, 120, 255), 25);    // azurová
  colorWipe(strip.Color(255, 255, 255), 25);  // bílá (pozor na proud)

  // 4) Odraz („bounce“) – „koule“ skáče tam a zpět
  bounce(strip.Color(255, 0, 120), 50);

  // 5) Jiskření na tmavém pozadí
  sparkle(strip.Color(0, 5, 20), 20);         // tmavě modré pozadí + náhodné „jiskry“
}

// =================== Implementace efektů ===================

// Efekt: plynulá duha ve všech pixelech (rotace barevného kolečka)
void rainbow(uint8_t wait) {
  for (uint16_t j = 0; j < 256 * 3; j++) {    // projedeme kolečko několikrát
    for (uint16_t i = 0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, wheel((i * 256 / strip.numPixels() + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// Efekt: „theater chase“ – běžící tečky
void theaterChase(uint32_t c, uint8_t wait) {
  for (int cycle = 0; cycle < 20; cycle++) {
    for (int offset = 0; offset < 3; offset++) {
      strip.clear();
      for (uint16_t i = offset; i < strip.numPixels(); i += 3) {
        strip.setPixelColor(i, c);
      }
      strip.show();
      delay(wait);
    }
  }
}

// Efekt: postupné rozsvěcení celé řady jednou barvou
void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
  delay(250);
}

// Efekt: „odrážející se koule“
void bounce(uint32_t c, uint8_t wait) {
  int pos = 0;
  int dir = 1;
  for (int steps = 0; steps < 4 * strip.numPixels(); steps++) {
    strip.clear();
    strip.setPixelColor(pos, c);
    // slabé dosvití sousedů pro hezčí efekt
    if (pos - 1 >= 0) strip.setPixelColor(pos - 1, strip.Color(10, 10, 10));
    if (pos + 1 < (int)strip.numPixels()) strip.setPixelColor(pos + 1, strip.Color(10, 10, 10));

    strip.show();
    delay(wait);

    pos += dir;
    if (pos <= 0 || pos >= (int)strip.numPixels() - 1) dir = -dir; // odraz
  }
}

// Efekt: náhodné jiskry na tmavém pozadí
void sparkle(uint32_t baseColor, uint8_t wait) {
  // pozadí
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, baseColor);
  }
  strip.show();

  // jiskření
  for (int k = 0; k < 120; k++) { // délka efektu
    int idx = random(strip.numPixels());
    uint32_t old = strip.getPixelColor(idx);
    strip.setPixelColor(idx, strip.Color(255, 255, 255)); // jiskra
    strip.show();
    delay(wait);
    strip.setPixelColor(idx, old); // návrat k pozadí
  }
}

// Pomocná funkce: převod pozice 0–255 na RGB po barevném kolečku
uint32_t wheel(byte pos) {
  if (pos < 85) {
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  } else if (pos < 170) {
    pos -= 85;
    return strip.Color(255 - pos * 3, 0, pos * 3);
  } else {
    pos -= 170;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
}
