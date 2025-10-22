// Two LEDs + button using millis()
// LED1 blinks every second (non-blocking)
// LED2 toggles on button press instantly

const int LED1 = 3;
const int LED2 = 4;
const int BTN  = 2;

unsigned long prevMillis = 0;
const unsigned long interval = 1000;
bool led1State = false;

int led2State = LOW;
int lastBtn = HIGH;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("MILLIS version running...");
}

void loop() {
  unsigned long now = millis();

  // --- LED1 blink using millis ---
  if (now - prevMillis >= interval) {
    prevMillis = now;
    led1State = !led1State;
    digitalWrite(LED1, led1State);
  }

  // --- LED2 toggle on button press ---
  int btn = digitalRead(BTN);
  if (lastBtn == HIGH && btn == LOW) {
    led2State = !led2State;
    digitalWrite(LED2, led2State);
    Serial.println("Button pressed!");
  }
  lastBtn = btn;
}
