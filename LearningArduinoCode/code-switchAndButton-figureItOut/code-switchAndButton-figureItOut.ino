// 🔹 DEMO 2: Button cycles through modes
// Predict: what will happen when you press the button repeatedly?

const int BTN = 2;
const int LED = 3;
int mode = 0;
int lastBtn = HIGH;

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  int btn = digitalRead(BTN);
  if (lastBtn == HIGH && btn == LOW) {
    mode = (mode + 1) % 4;  // cycles 0–3
    Serial.print("Mode: ");
    Serial.println(mode);
  }
  lastBtn = btn;

  switch (mode) {
    case 0: digitalWrite(LED, LOW); break;                      // off
    case 1: digitalWrite(LED, HIGH); break;                     // on
    case 2: digitalWrite(LED, millis() / 500 % 2); break;       // blink
    case 3: analogWrite(LED, (millis() / 4) % 256); break;      // fade
  }
}
