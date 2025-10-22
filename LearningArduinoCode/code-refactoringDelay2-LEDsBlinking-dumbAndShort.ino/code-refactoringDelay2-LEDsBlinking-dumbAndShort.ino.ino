// Three LEDs blinking with different speeds using only delay()
// The full pattern repeats every 2 seconds

const int LED1 = 2;   // Blinks every 1 second
const int LED2 = 3;   // Blinks every 200 ms
const int LED3 = 4;   // Blinks every 500 ms

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
}

void loop() {
  // We'll divide 2 seconds into small steps (every 100 ms)
  // and decide at each step which LEDs should toggle.

  for (int t = 0; t < 2000; t += 100) {   // total period: 2000 ms
    // LED1 toggles every 1000 ms
    if (t % 1000 == 0) digitalWrite(LED1, !digitalRead(LED1));

    // LED2 toggles every 200 ms
    if (t % 200 == 0) digitalWrite(LED2, !digitalRead(LED2));

    // LED3 toggles every 500 ms
    if (t % 500 == 0) digitalWrite(LED3, !digitalRead(LED3));

    delay(100);
  }
}
