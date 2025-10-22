// Toggle LED on press – debounced and printed to Serial
const int BTN = 2;
const int LED = 3;

const unsigned long DEBOUNCE_MS = 30;

int rawPrev = HIGH;
int stableState = HIGH;
int stablePrev = HIGH;
unsigned long lastChange = 0;

int ledState = LOW;

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Starting: toggle LED each press (debounced)");
}

void loop() {
  unsigned long now = millis();
  int raw = digitalRead(BTN);

  // if raw input changed, restart timer
  if (raw != rawPrev) {
    rawPrev = raw;
    lastChange = now;
  }

  // if input stable long enough, update debounced state
  if ((now - lastChange) > DEBOUNCE_MS) {
    if (stableState != raw) {
      stableState = raw;

      // detect press (HIGH → LOW)
      if (stablePrev == HIGH && stableState == LOW) {
        ledState = !ledState;
        digitalWrite(LED, ledState);
        Serial.print("Button pressed → LED ");
        Serial.println(ledState ? "ON" : "OFF");
      }

      stablePrev = stableState;
    }
  }
}
