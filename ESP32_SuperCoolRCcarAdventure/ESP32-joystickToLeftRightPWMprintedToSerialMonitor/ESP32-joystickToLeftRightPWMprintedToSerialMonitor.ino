/******************************************************
ZAPOJENÍ (ESP32 + L298N + Joystick)
-------------------------------------------------------
L298N (2× DC motor):
- IN1 -> GPIO 18  (směr Motor A)
- IN2 -> GPIO 19
- ENA -> GPIO 23  (rychlost Motor A – PWM)
- IN3 -> GPIO 5   (směr Motor B)
- IN4 -> GPIO 17
- ENB -> GPIO 22  (rychlost Motor B – PWM)

Joystick:
- VRY (Y osa – dopředu/dozadu) -> GPIO 34
- VRX (X osa – zatáčení)       -> GPIO 35
- VCC -> 3V3 (nebo 5V dle modulu)
- GND -> GND (společná zem s ESP32)

POZNÁMKA:
- Sdílej GND mezi ESP32, L298N a joystickem.
- Motory napájej zvlášť (např. 6–12V na L298N VMOTOR).
- Tento kód zatím jen počítá a vypisuje PWM hodnoty – motory se netočí.

Úkoly pro pochopení kódu:
1) Co znamená L = Y + X a R = Y - X?
2) Co dělá funkce normalizeLR()? Proč je důležitá?
3) Jak bys přidal deadband, aby joystick kolem středu nic nedělal?
4) Jaký vliv má invertování osy Y?
5) Zkus připojit L298N a doplnit writeMotor() – kam bys to vložil?

******************************************************/

#include <Arduino.h>

// ==== Piny joysticku ====
const int PIN_JOY_Y = 34;  // throttle (dopředu/dozadu)
const int PIN_JOY_X = 35;  // steering (zatáčení)

// ==== Pomocná funkce: přemapování na -1..+1 ====
float readNormalized(int pin, bool invert = false) {
  int raw = analogRead(pin);      // 0–4095
  float val = (raw - 2048) / 2048.0f;  // -1..+1
  if (invert) val = -val;
  if (val > 1.0f) val = 1.0f;
  if (val < -1.0f) val = -1.0f;
  return val;
}

// ==== Normalizace (aby se kola nevešla přes ±1) ====
void normalizeLR(float &L, float &R) {
  float m = max(fabsf(L), fabsf(R));
  if (m > 1.0f) { L /= m; R /= m; }
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // ESP32 ADC: 0..4095
}

void loop() {
  // --- Čtení joysticku ---
  float Y = readNormalized(PIN_JOY_Y, true); // invertuj Y (nahoru = dopředu)
  float X = readNormalized(PIN_JOY_X, false);

  // --- Výpočet levého a pravého motoru (arcade mix) ---
  float L = Y + X;
  float R = Y - X;
  normalizeLR(L, R);

  // --- Výpis ---
  Serial.printf("X=% .2f  Y=% .2f  |  L=% .2f  R=% .2f\n", X, Y, L, R);

  delay(100);
}
