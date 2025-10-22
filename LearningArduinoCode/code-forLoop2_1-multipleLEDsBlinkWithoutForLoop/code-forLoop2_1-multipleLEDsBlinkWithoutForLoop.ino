// Blikání třemi LED bez for cyklu
// -----------------------------------------------------------
// 🧩 Úkol 1: Přidej další LED
//    ➜ Přidej novou LED na další pin a rozblikaj ji spolu s ostatními
// -----------------------------------------------------------
// 🧩 Úkol 2: Změň rychlost blikání
// -----------------------------------------------------------
// 🧩 Úkol 3: Udělej, aby blikaly postupně (ne najednou)
// -----------------------------------------------------------


const int LED1 = 2;
const int LED2 = 3;
const int LED3 = 4;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
}

void loop() {
  // Zapnout všechny LED
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  delay(500);

  // Vypnout všechny LED
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  delay(500);
}
