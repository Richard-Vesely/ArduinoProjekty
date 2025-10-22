// Two LEDs + button using delay()
// Blink period for LED1 can be changed from Serial Monitor

const int LED1 = 3;
const int LED2 = 4;
const int BTN  = 2;

int led2State = LOW;
int lastBtn = HIGH;
int period = 1000; // default 1 second

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Type a number (ms) to change LED1 period:");
}

void loop() {
  // --- LED1 blink ---
  digitalWrite(LED1, HIGH);
  delay(period / 2);
  digitalWrite(LED1, LOW);
  delay(period / 2);

  // --- LED2 on button press ---
  int btn = digitalRead(BTN);
  if (lastBtn == HIGH && btn == LOW) {
    led2State = !led2State;
    digitalWrite(LED2, led2State);
    Serial.println("Button pressed!");
  }
  lastBtn = btn;

  // --- Read new period from Serial ---
  if (Serial.available() > 0) {
    int newPeriod = Serial.parseInt();
    if (newPeriod > 0) {
      period = newPeriod;
      Serial.print("New blink period: ");
      Serial.print(period);
      Serial.println(" ms");
    }
    Serial.readStringUntil('\n'); // clear buffer
  }
}
