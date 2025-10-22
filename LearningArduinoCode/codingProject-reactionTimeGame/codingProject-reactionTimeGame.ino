// Reaction Game: press to start, wait for LED, press as fast as possible
const int LED = 8;
const int BTN = 7;
// Optional buzzer: const int BUZ = 9;

void waitForButtonRelease() {
  while (digitalRead(BTN) == LOW) { /* wait */ }
  delay(20); // debounce
}

void waitForButtonPress() {
  while (digitalRead(BTN) == HIGH) { /* wait */ }
  delay(20); // debounce
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  // pinMode(BUZ, OUTPUT);
  Serial.begin(9600);
  randomSeed(analogRead(A0)); // crude seed
  Serial.println("Reaction Game");
  Serial.println("Press button to start a round.");
}

void loop() {
  // Wait for player to start
  if (digitalRead(BTN) == LOW) {
    waitForButtonRelease();
    Serial.println("Get ready...");

    // Random wait 1–4 seconds; detect early press = false start
    unsigned long waitMs = random(1000, 4001);
    unsigned long startWait = millis();
    while (millis() - startWait < waitMs) {
      if (digitalRead(BTN) == LOW) {
        Serial.println("False start! Wait for the LED.");
        // brief penalty flash
        digitalWrite(LED, HIGH); delay(150);
        digitalWrite(LED, LOW);  delay(400);
        return; // restart round
      }
    }

    // Signal: LED on (player should press ASAP)
    digitalWrite(LED, HIGH);
    // tone(BUZ, 1200, 80);

    unsigned long t0 = millis();
    waitForButtonPress();
    unsigned long rt = millis() - t0;

    digitalWrite(LED, LOW);
    Serial.print("Reaction time: ");
    Serial.print(rt);
    Serial.println(" ms");

    // Wait for release before next round
    waitForButtonRelease();
    Serial.println("Press button to play again.");
  }
}
