/****************************************************
💡 LED BLINKER — FUNCTION VERSION
Each LED is described by parallel arrays.
A single function handles blinking logic.
****************************************************/

const int pins[]      = {3, 4, 5};
const int onTimes[]   = {200, 500, 1000};
const int offTimes[]  = {800, 500, 200};
bool states[]         = {false, false, false};
unsigned long prevs[] = {0, 0, 0};

const int ledCount = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(9600);
  Serial.println("=== FUNCTION LED BLINKER ===");
  for (int i = 0; i < ledCount; i++) pinMode(pins[i], OUTPUT);
}

void loop() {
  unsigned long currentMillis = millis();

  for (int i = 0; i < ledCount; i++) {
    updateLED(i, currentMillis);
  }
}

// 🔹 Function that updates one LED
void updateLED(int index, unsigned long now) {
  unsigned long interval = states[index] ? onTimes[index] : offTimes[index];
  if (now - prevs[index] >= interval) {
    states[index] = !states[index];
    prevs[index] = now;
    digitalWrite(pins[index], states[index]);
  }
}
