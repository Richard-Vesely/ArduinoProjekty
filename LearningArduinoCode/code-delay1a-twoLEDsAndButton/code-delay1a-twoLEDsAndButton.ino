// Two LEDs + button using delay()
// LED1 blinks every second
// LED2 toggles when button is pressed

const int LED1 = 3;     // blinking LED
const int LED2 = 4;     // toggled by button
const int BTN  = 2;     // button to GND

int led2State = LOW;
int lastBtn = HIGH;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("DELAY version running...");
}

void loop() {
  // --- LED1 blink with delay ---
  digitalWrite(LED1, HIGH);
  delay(500);
  digitalWrite(LED1, LOW);
  delay(500);

  // --- Button check (but only runs between delays!) ---
  int btn = digitalRead(BTN);
  if (lastBtn == HIGH && btn == LOW) {
    led2State = !led2State;
    digitalWrite(LED2, led2State);
    Serial.println("Button pressed!");
  }
  lastBtn = btn;
}
