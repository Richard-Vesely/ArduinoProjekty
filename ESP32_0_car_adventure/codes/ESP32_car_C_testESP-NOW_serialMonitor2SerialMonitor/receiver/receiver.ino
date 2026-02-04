/*
========================================
ZAPOJENÍ (jednoduše, pro děti):
----------------------------------------
NeoPixel Ring (12 LED, WS2812):
- VCC  →  5V na Arduinu
- GND  →  GND na Arduinu
- DIN  →  pin 6 na Arduinu

POZOR:
- Vždy propojit GND Arduina a GND LED!
- Nepřipojuj / neodpojuj LED, když je Arduino zapnuté.

========================================
Úkoly pro pochopení kódu:
----------------------------------------
1) Která proměnná říká, kolik má ring LED?
2) Co se stane, když změníš pin LED z 6 na jiný?
3) Kde se přepínají jednotlivé světelné módy?
4) Jak bys přidal 6. mód?
5) Co dělá funkce strip.show()?
========================================
*/

#include <Adafruit_NeoPixel.h>

#define LED_PIN    6
#define LED_COUNT  12

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

int mode = 0;
unsigned long lastChange = 0;
const unsigned long modeDuration = 4000; // 4 sekundy na jeden efekt

void setup() {
  strip.begin();
  strip.setBrightness(80);
  strip.show(); // zhasnout vše
}

void loop() {
  // automatické přepínání módů
  if (millis() - lastChange > modeDuration) {
    mode = (mode + 1) % 5;
    lastChange = millis();
  }

  switch (mode) {
    case 0: rainbowSpin(); break;
    case 1: colorWipeBlue(); break;
    case 2: theaterChaseRed(); break;
    case 3: breathingPurple(); break;
    case 4: randomSparkles(); break;
  }
}

/* ================== EFEKTY ================== */

void rainbowSpin() {
  static int hue = 0;
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue + i * 5000)));
  }
  strip.show();
  hue += 200;
  delay(40);
}

void colorWipeBlue() {
  static int index = 0;
  strip.clear();
  strip.setPixelColor(index, strip.Color(0, 0, 255));
  strip.show();
  index = (index + 1) % LED_COUNT;
  delay(120);
}

void theaterChaseRed() {
  static int offset = 0;
  strip.clear();
  for (int i = offset; i < LED_COUNT; i += 3) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
  offset = (offset + 1) % 3;
  delay(100);
}

void breathingPurple() {
  static int brightness = 0;
  static int direction = 1;

  strip.setBrightness(brightness);
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(150, 0, 200));
  }
  strip.show();

  brightness += direction * 5;
  if (brightness <= 5 || brightness >= 100) direction = -direction;
  delay(30);
}

void randomSparkles() {
  strip.clear();
  int pixel = random(LED_COUNT);

  strip.setPixelColor(
    pixel,
    strip.Color(random(255), random(255), random(255))
  );

  strip.show();
  delay(80);
}

