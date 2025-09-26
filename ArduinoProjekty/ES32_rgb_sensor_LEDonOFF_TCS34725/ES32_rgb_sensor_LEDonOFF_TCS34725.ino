/*
ESP32 + TCS34725 with LED control via Serial ("on"/"off")

I2C wiring:
  ESP32 3V3  → VIN (or VCC) on TCS34725
  ESP32 GND  → GND
  ESP32 GPIO21 (SDA) → SDA
  ESP32 GPIO22 (SCL) → SCL

Optional LED control (for clones with LED/LED_EN pad):
  ESP32 GPIO4 → LED (or LED_EN) pad on the breakout
  (If the LED is hardwired ON via a solder jumper, cut that jumper first.)

Serial: 115200 baud. Type: on / off
*/

#include <Wire.h>
#include "Adafruit_TCS34725.h"

// I2C config
Adafruit_TCS34725 tcs(
  TCS34725_INTEGRATIONTIME_154MS,
  TCS34725_GAIN_4X
);

// ===== Optional direct LED control pin (set to -1 to disable) =====
#define LED_CTRL_PIN 4   // connect to breakout's LED / LED_EN pad; set -1 if not used

bool ledOn = true;

void setLed(bool on) {
  ledOn = on;

  // Try library LED control (works on Adafruit-style boards; inverted!)
  // false = LED ON, true = LED OFF
  tcs.setInterrupt(!on);

  // Also try direct pin control for clones
  #if (LED_CTRL_PIN >= 0)
    // Many boards use active-HIGH to turn LED ON; some are active-LOW.
    // We'll try active-HIGH first. If your LED inverts, swap the writes.
    digitalWrite(LED_CTRL_PIN, on ? HIGH : LOW);
  #endif

  Serial.print("[LED] "); Serial.println(on ? "ON" : "OFF");
}

static inline uint8_t clamp255(float v){
  if (v < 0) return 0;
  if (v > 255) return 255;
  return (uint8_t)(v + 0.5f);
}

void rawToRGB255(uint16_t r, uint16_t g, uint16_t b, uint16_t c,
                 uint8_t &R, uint8_t &G, uint8_t &B) {
  float denom = (c > 0) ? c : (float)max<uint16_t>(1, max(r, max(g, b)));
  float k = 255.0f / denom;
  R = clamp255(r * k);
  G = clamp255(g * k);
  B = clamp255(b * k);
}

String inbuf;

void handleSerial() {
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;
    if (ch == '\n') {
      inbuf.trim();
      if (inbuf.equalsIgnoreCase("on"))       setLed(true);
      else if (inbuf.equalsIgnoreCase("off")) setLed(false);
      else if (!inbuf.isEmpty()) {
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
  delay(200);
  Serial.println("TCS34725 RGB with LED control (type 'on' / 'off')");

  #if (LED_CTRL_PIN >= 0)
    pinMode(LED_CTRL_PIN, OUTPUT);
    digitalWrite(LED_CTRL_PIN, HIGH); // assume active-HIGH ON; we sync below
  #endif

  if (!tcs.begin()) {
    Serial.println("ERROR: TCS34725 not found. Check wiring.");
    while (1) delay(1000);
  }

  // Start with LED ON for stable readings
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

  for (int i=0;i<50;i++){ handleSerial(); delay(10); }
}
