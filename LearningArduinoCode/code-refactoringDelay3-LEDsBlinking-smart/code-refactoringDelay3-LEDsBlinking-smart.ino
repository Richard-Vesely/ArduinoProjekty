// SMART version: 3 LEDs blinking independently at different speeds
// LED1: 1 second, LED2: 200 ms, LED3: 500 ms
// Full pattern repeats naturally without blocking

const int LED1 = 2;
const int LED2 = 3;
const int LED3 = 4;

const unsigned long interval1 = 1000;  // LED1 blinks every 1 second
const unsigned long interval2 = 200;   // LED2 blinks every 0.2 seconds
const unsigned long interval3 = 500;   // LED3 blinks every 0.5 seconds

unsigned long prev1 = 0;
unsigned long prev2 = 0;
unsigned long prev3 = 0;

bool state1 = false;
bool state2 = false;
bool state3 = false;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
}

void loop() {
  unsigned long now = millis();

  // LED1 every 1000 ms
  if (now - prev1 >= interval1) {
    prev1 = now;
    state1 = !state1;
    digitalWrite(LED1, state1);
  }

  // LED2 every 200 ms
  if (now - prev2 >= interval2) {
    prev2 = now;
    state2 = !state2;
    digitalWrite(LED2, state2);
  }

  // LED3 every 500 ms
  if (now - prev3 >= interval3) {
    prev3 = now;
    state3 = !state3;
    digitalWrite(LED3, state3);
  }
}
