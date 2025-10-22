/****************************************************
💡 3× LED BLINKER — INDIVIDUAL + ARRAY OF STRUCTS
-----------------------------------------------------
You can define each LED separately (for clarity)
and then still handle them all in one loop.
****************************************************/

struct LED {
  int pin;
  unsigned long onTime;
  unsigned long offTime;
  bool state;
  unsigned long previous;
};

// 🔹 1. Individual LEDs (easy to tweak each)
LED led1 = {3, 200, 800, false, 0};
LED led2 = {4, 500, 500, false, 0};
LED led3 = {5, 1000, 200, false, 0};

// 🔹 2. Make an array of them (so we can loop)
LED leds[] = {led1, led2, led3};
const int ledCount = sizeof(leds) / sizeof(leds[0]);

// 🔹 3. Function for updating any LED
void updateLED(LED &led) {
  unsigned long now = millis();
  unsigned long interval = led.state ? led.onTime : led.offTime;

  if (now - led.previous >= interval) {
    led.state = !led.state;
    led.previous = now;
    digitalWrite(led.pin, led.state);
  }
}

void setup() {
  Serial.begin(9600);
  Serial.println("=== STRUCT + ARRAY LED BLINKER ===");

  for (int i = 0; i < ledCount; i++) {
    pinMode(leds[i].pin, OUTPUT);
  }
}

void loop() {
  for (int i = 0; i < ledCount; i++) {
    updateLED(leds[i]);
  }
}
