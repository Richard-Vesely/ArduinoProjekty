// Toggle LED on each button press – prints to Serial
const int BTN = 2;
const int LED = 3;

int lastBtn = HIGH;  // last button state
int ledState = LOW;  // LED memory

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Starting: toggle LED each press (no debounce)");
}

void loop() {
  int btn = digitalRead(BTN);

  // detect press edge (HIGH → LOW)
  if (lastBtn == HIGH && btn == LOW) {
    ledState = !ledState;
    digitalWrite(LED, ledState);
    Serial.print("Button pressed → LED ");
    Serial.println(ledState ? "ON" : "OFF");
  }

  lastBtn = btn;
}
