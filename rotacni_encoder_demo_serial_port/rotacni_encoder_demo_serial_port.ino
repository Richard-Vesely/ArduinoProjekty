/*
==================== ZAPOJENÍ (Arduino Leonardo + rotační encoder) ====================

Typický mechanický enkodér má 3 piny pro signál (GND, A, B) a 2 piny pro tlačítko (BTN, GND).

Arduino Leonardo  ->  ENCODER modul
-----------------------------------------------------
GND               ->  GND (společná zem pro enkodér i tlačítko)
D2                ->  A (výstup A z enkodéru)
D3                ->  B (výstup B z enkodéru)
D4                ->  BTN (spíná proti GND – tlačítko v enkodéru)

Poznámky:
- Piny D2 a D3 jsou vhodné: umí externí přerušení na Leonardo (stabilní čtení).
- Na piny A/B a BTN v kódu zapneme interní PULLUP rezistory.
- Elektronika enkodéru je beznapěťová (jen spínače), napájení nepotřebuje (pokud nemá vlastní LED).
=======================================================================================
*/

#include <Encoder.h>
#include <Bounce2.h>

// Piny
const uint8_t PIN_ENC_A = 2;   // enkodér A
const uint8_t PIN_ENC_B = 3;   // enkodér B
const uint8_t PIN_BTN   = 4;   // tlačítko (spíná na GND)

// Vytvoření objektů
Encoder enc(PIN_ENC_A, PIN_ENC_B);
Bounce  btn = Bounce();

volatile long position = 0;    // aktuální pozice (v "krocích")
long lastReported = 0;         // poslední vypsaná hodnota

// Volitelně: nastav "rozlišení" – kolik kroků tvoří jednu "klik" (záleží na enkodéru):
// Pokud cítíš 20 detentů na otáčku, ale knihovna vrací 80 kroků, nastav si scale=4 atd.
const int SCALE = 1;

void setup() {
  // Vstupy s pull-up (A/B i tlačítko jdou na GND při sepnutí)
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BTN,   INPUT_PULLUP);

  // Bounce2 pro tlačítko
  btn.attach(PIN_BTN);
  btn.interval(10); // debounce ~10ms

  Serial.begin(115200);
  while (!Serial) { /* Leonardo čeká na USB */ }

  // Nulování na startu
  enc.write(0);
  position = 0;

  Serial.println(F("Rotary Encoder Demo (Leonardo)"));
  Serial.println(F("Otacej enkoderem, stisk resetuje na 0."));
}

void loop() {
  // --- ČTENÍ ENKODÉRU ---
  long raw = enc.read();

  // Vynásob/označ kroky dle škálování (volitelné)
  long scaled = raw / SCALE;

  if (scaled != lastReported) {
    lastReported = scaled;
    Serial.print(F("Pozice: "));
    Serial.println(scaled);
  }

  // --- TLAČÍTKO S DEBOUNCEM ---
  btn.update();
  if (btn.fell()) {              // stisk (přechod z HIGH -> LOW)
    enc.write(0);
    lastReported = 0;
    Serial.println(F("Tlačítko: RESET na 0"));
  }

  // (Volitelné) Můžeš přidat soft-limity, např. od -50 do +50:
  // if (scaled > 50) { enc.write(50 * SCALE); }
  // if (scaled < -50){ enc.write(-50 * SCALE); }

  // Krátká prodleva pro klidnější výpis (ne nutná)
  delay(1);
}

/*
Tipy:
- Pokud hodnoty poskakují příliš, zkrať kabely a zpomal čtení výpisu (větší delay),
  případně nastav SCALE=2/4 podle mechaniky enkodéru (méně výpisů na „cvak“).
- Jestli se otáčení chová „obráceně“, prohoď PIN_ENC_A a PIN_ENC_B, nebo použij záporné SCALE.
- Pokud máš modul s vestavěnými rezistory/filtry, nech INPUT_PULLUP klidně zapnutý – nevadí.
*/
