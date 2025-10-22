// Blikání třemi LED pomocí for cyklu
// -----------------------------------------------------------
// 🧩 Úkol 1: Přidej další LED
// -----------------------------------------------------------
// 🧩 Úkol 2: Změň rychlost blikání
// -----------------------------------------------------------
// 🧩 Úkol 3: Udělej, aby blikaly postupně (ne všechny najednou)
// -----------------------------------------------------------
// 🧩 Úkol 4: Zkus změnit pořadí blikání
// -----------------------------------------------------------
// 🧩 Úkol 5: Vysvětli, proč je tento kód kratší než ten bez for cyklu
// -----------------------------------------------------------
const int ledCount = 3;

void setup() {
  for (int i = 0; i < ledCount; i++) {
    pinMode(leds[i], OUTPUT);
  }
}

void loop() {
  // Turn all ON
  for (int i = 0; i < ledCount; i++) {
    digitalWrite(leds[i], HIGH);
  }
  delay(500);

  // Turn all OFF
  for (int i = 0; i < ledCount; i++) {
    digitalWrite(leds[i], LOW);
  }
  delay(500);
}
