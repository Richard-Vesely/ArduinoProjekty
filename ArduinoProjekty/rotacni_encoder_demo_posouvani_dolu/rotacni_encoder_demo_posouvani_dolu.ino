/*
================= ZAPOJENÍ (Arduino Leonardo + rotační enkodér) =================

Varianta: „holý“ mechanický enkodér (bez elektroniky)
Piny enkodéru: CLK (A), DT (B), SW (tlačítko), GND

Arduino Leonardo  ->  ENKODÉR
--------------------------------
GND               ->  GND
D2 (interrupt)    ->  CLK (A)
D3 (interrupt)    ->  DT (B)
D4                ->  SW (tlačítko spíná na GND)

Poznámky:
- V kódu používáme INPUT_PULLUP: piny mají v klidu HIGH, sepnuto = LOW.
- Máš-li enkodér modul s pinem „+“ (VCC pro vlastní LED/pull-upy), připoj jej na 5V.
==================================================================================
*/

#include <Encoder.h>
#include <Bounce2.h>
#include <Keyboard.h>

const uint8_t PIN_ENC_A = 2;   // CLK (kanál A)
const uint8_t PIN_ENC_B = 3;   // DT  (kanál B)
const uint8_t PIN_BTN   = 4;   // SW  (tlačítko)

Encoder enc(PIN_ENC_A, PIN_ENC_B);
Bounce  btn = Bounce();

long lastSteps = 0;

// U spousty levných enkodérů vychází 4 „kroky“ knihovny na jeden hmatový cvak.
// Pokud vidíš 4x více kroků, nastav SCALE=4. Když ne, dej SCALE=1.
const int SCALE = 2;

// Mírné omezení, aby se neposílalo příliš mnoho kláves v jedné smyčce
const int MAX_KEYS_PER_LOOP = 5;

void setup() {
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);
  pinMode(PIN_BTN,   INPUT_PULLUP);

  btn.attach(PIN_BTN);
  btn.interval(8); // debounce

  Keyboard.begin();
  enc.write(0);
  lastSteps = 0;
}

void loop() {
  // --- ENKODÉR → šipky ---
  long raw = enc.read();
  long steps = raw / SCALE;
  long delta = steps - lastSteps;

  if (delta != 0) {
    // Omezíme počet opakování na smyčku, ať to nespamuje
    int reps = (delta > 0) ? (delta > MAX_KEYS_PER_LOOP ? MAX_KEYS_PER_LOOP : delta)
                           : (-delta > MAX_KEYS_PER_LOOP ? MAX_KEYS_PER_LOOP : -delta);

    for (int i = 0; i < reps; i++) {
      if (delta > 0) {
        // otočení „doprava“ → šipka dolů
        Keyboard.write(KEY_DOWN_ARROW);
      } else {
        // otočení „doleva“ → šipka nahoru
        Keyboard.write(KEY_UP_ARROW);
      }
      delay(2); // krátké zpoždění mezi klávesami
    }

    // posunuli jsme část delta; upravíme lastSteps
    lastSteps += (delta > 0 ? reps : -reps);
  }

  // --- TLAČÍTKO → Enter ---
  btn.update();
  if (btn.fell()) {
    Keyboard.write(KEY_RETURN);
  }

  delay(1);
}
