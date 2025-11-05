// Button directly controls LED – prints state to Serial
const int BTN = 2;   // button between pin 2 and GND
const int LED = 3;

void setup() {
  pinMode(BTN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  Serial.println("Starting: LED follows button (hold = ON)");
}

void loop() {
  int btn = digitalRead(BTN);

  if (btn == LOW) {
    digitalWrite(LED, LOW);
    Serial.println("Button pressed → LED ON");
  } else {
    digitalWrite(LED, HIGH);
    Serial.println("Button released → LED OFF");
  }
}
