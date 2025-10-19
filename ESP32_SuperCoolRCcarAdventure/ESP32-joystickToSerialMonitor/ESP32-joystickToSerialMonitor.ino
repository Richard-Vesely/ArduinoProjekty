/******************************************************
ZAPOJENÍ (ESP32 + Joystick)
-------------------------------------------------------
Joystick modul:
- VRY (Y osa – dopředu/dozadu) -> GPIO 34 (ADC1_CH6)
- VRX (X osa – zatáčení)       -> GPIO 35 (ADC1_CH7)
- VCC -> 3V3  (nebo 5V podle modulu)
- GND -> GND (společná zem s ESP32)

POZNÁMKA:
Joystick vrací napětí 0–3.3 V. Střed je přibližně 1.65 V (ADC ≈ 2048).

Úkoly pro pochopení kódu:
1) Jaký je rozdíl mezi hodnotami „raw“ (0–4095) a „normalized“ (−1..+1)?
2) Proč je osa Y invertovaná (nahoru = +1)?
3) Co by se stalo, kdyby joystick nebyl přesně uprostřed při uvolnění?
4) Zkus přidat „deadband“, aby malé pohyby kolem středu ignoroval.
5) Můžeš přidat průměrování pro plynulejší čtení?

******************************************************/

#include <Arduino.h>

// Piny joysticku
const int PIN_JOY_Y = 34;  // throttle
const int PIN_JOY_X = 35;  // steering

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // ESP32 ADC 0–4095
}

void loop() {
  // 1. Přečti hodnoty z joysticku
  int rawX = analogRead(PIN_JOY_X);
  int rawY = analogRead(PIN_JOY_Y);

  // 2. Přemapuj 0–4095 na −1..+1
  float normX = (rawX - 2048) / 2048.0f;
  float normY = (2048 - rawY) / 2048.0f; // invertujeme osu Y, aby dopředu bylo +1

  // 3. Omez na rozsah (kvůli tolerancím)
  if (normX > 1.0f) normX = 1.0f;
  if (normX < -1.0f) normX = -1.0f;
  if (normY > 1.0f) normY = 1.0f;
  if (normY < -1.0f) normY = -1.0f;

  // 4. Vypiš do Serial Monitoru
  Serial.printf("X: % .2f\tY: % .2f\t(rawX=%4d, rawY=%4d)\n", normX, normY, rawX, rawY);

  delay(100);
}
