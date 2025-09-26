/*
========================================
Jak to zapojit (Arduino Leonardo + TCS34725)
========================================
Napájení:
- TCS34725 VIN  -> 5V (nebo 3V3) na Arduinu
- TCS34725 GND  -> GND na Arduinu

I2C (Leonardo):
- TCS34725 SDA  -> pin SDA na Arduinu (označený "SDA", zároveň D2)
- TCS34725 SCL  -> pin SCL na Arduinu (označený "SCL", zároveň D3)

Volitelné: řízení bílé LED na modulu (pokud má pin "LED" / "LED_EN"):
- TCS34725 LED  -> D4 na Arduinu (v kódu lze vypnout, pokud nepřipojíte)

Poznámky:
- Některé moduly mají LED napevno zapnutou propojkou. Pokud chcete LED řídit,
  je potřeba tu propojku rozpojit (podle dokumentace modulu).
- Rychlost sériové linky: 115200. Do Serial Monitoru pište: on / off.

========================================
Úkoly, pro pochopení kódu (pro děti)
========================================
1) Co dělá funkce setLed(true/false)? Zkus "on" a "off" v Serial Monitoru.
2) Co znamenají čísla RAW R, G, B, C? Kdy se mění nejvíc?
3) Přikryj senzor rukou. Jak se změní „Lux“ a „CT(K)“?
4) Změň v kódu zesílení (GAIN_1X, 4X, 16X, 60X). Co se stane?
5) Proč dáváme delší integrační čas (600 ms)? Co to dělá s kvalitou měření?
*/

#include <Wire.h>
#include "Adafruit_TCS34725.h"

// --- Nastavení senzoru (PRAVIDLO: používej 600 ms, ne 700 ms) ---
Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_600MS,
  TCS34725_GAIN_4X
);

// ===== Volitelný pin pro přímé řízení LED na breakout modulu =====
// Pokud nepoužíváte, dejte -1
#define LED_CTRL_PIN 4

bool ledOn = true;

static inline uint8_t clamp255(float v){
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)(v + 0.5f);
}

// jednoduché max z trojice (bez šablon)
static inline uint16_t max3u16(uint16_t a, uint16_t b, uint16_t c) {
  uint16_t m = a;
  if (b > m) m = b;
  if (c > m) m = c;
  return m;
}

// Převod surových dat na 0–255 RGB podle podílu v „čirém“ kanálu C
void rawToRGB255(uint16_t r, uint16_t g, uint16_t b, uint16_t c,
                 uint8_t &R, uint8_t &G, uint8_t &B) {
  uint16_t base = (c > 0) ? c : max3u16(r, g, b);
  if (base < 1) base = 1;
  float k = 255.0f / (float)base;
  R = clamp255(r * k);
  G = clamp255(g * k);
  B = clamp255(b * k);
}

void setLed(bool on) {
  ledOn = on;

  // U většiny modulů: setInterrupt(false) = LED ON, true = LED OFF
  tcs.setInterrupt(!on);

#if (LED_CTRL_PIN >= 0)
  pinMode(LED_CTRL_PIN, OUTPUT);
  // Většina modulů má LED aktivní-HIGH (HIGH = svítí).
  digitalWrite(LED_CTRL_PIN, on ? HIGH : LOW);
#endif

  Serial.print("[LED] "); Serial.println(on ? "ON" : "OFF");
}

String inbuf;

void handleSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      // trim:
      while (inbuf.length() && (inbuf[inbuf.length()-1] == ' ' || inbuf[inbuf.length()-1] == '\t')) {
        inbuf.remove(inbuf.length()-1);
      }
      // zpracování příkazu
      if (inbuf.equalsIgnoreCase("on"))       setLed(true);
      else if (inbuf.equalsIgnoreCase("off")) setLed(false);
      else if (inbuf.length() > 0) {
        Serial.print("Unknown cmd: "); Serial.println(inbuf);
        Serial.println("Use: on / off");
      }
      inbuf = "";
    } else {
      inbuf += ch;
      if (inbuf.length() > 64) inbuf = "";
    }
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { /* počkej na USB CDC u Leonardo */ }
  delay(100);
  Serial.println("TCS34725 RGB (napiš 'on' / 'off' do Serial Monitoru)");

  if (!tcs.begin()) {
    Serial.println("ERROR: TCS34725 nenalezen. Zkontroluj zapojení.");
    while (1) delay(1000);
  }

  // Začni s LED zapnutou (stabilnější měření)
  setLed(true);
}

void loop() {
  handleSerial();

  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  uint8_t R, G, B;
  rawToRGB255(r, g, b, c, R, G, B);

  uint16_t ct  = tcs.calculateColorTemperature(r, g, b);
  uint16_t lux = tcs.calculateLux(r, g, b);

  Serial.print("RAW R:"); Serial.print(r);
  Serial.print(" G:");    Serial.print(g);
  Serial.print(" B:");    Serial.print(b);
  Serial.print(" C:");    Serial.print(c);
  Serial.print(" | RGB: ");
  Serial.print((int)R); Serial.print(",");
  Serial.print((int)G); Serial.print(",");
  Serial.print((int)B);
  Serial.print(" | CT(K): "); Serial.print(ct);
  Serial.print(" Lux: ");     Serial.print(lux);
  Serial.print(" | LED: ");   Serial.println(ledOn ? "ON" : "OFF");

  // ~10× za sekundu kontroluj příchozí příkazy
  for (int i=0;i<10;i++){ handleSerial(); delay(50); }
}
