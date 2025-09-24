/*
ESP32 → TCS34725 Color Sensor

WIRING (I2C):
--------------
ESP32 3V3  → VIN (or VCC) on TCS34725
ESP32 GND  → GND
ESP32 GPIO 21 (SDA) → SDA
ESP32 GPIO 22 (SCL) → SCL
*/

#include <Wire.h>
#include "Adafruit_TCS34725.h"

// Use integration time and gain supported by the official library
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_614MS, 
  TCS34725_GAIN_1X
);

void setup(void) {
  Serial.begin(115200);
  delay(200);
  Serial.println("TCS34725 test start...");

  if (tcs.begin()) {
    Serial.println("Sensor found!");
  } else {
    Serial.println("No TCS34725 found. Check wiring!");
    while (1) delay(100);
  }
}

void loop(void) {
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);

  Serial.print("R: "); Serial.print(r);
  Serial.print("  G: "); Serial.print(g);
  Serial.print("  B: "); Serial.print(b);
  Serial.print("  Clear: "); Serial.println(c);

  delay(500); // half a second
}
