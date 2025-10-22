// Co se bude dít?
const int leds[] = {2, 3, 4};
const int count = 3;

void setup() {
  for (int i = 0; i < count; i++) pinMode(leds[i], OUTPUT);
}

void loop() {
  for (int i = 0; i < count; i++) {          // for each LED
    for (int j = 0; j < 2; j++) {            // blink twice
      digitalWrite(leds[i], HIGH);
      delay(200);
      digitalWrite(leds[i], LOW);
      delay(200);
    }
    delay(400);
  }
}
