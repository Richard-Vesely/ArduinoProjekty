// Two LEDs + button using millis()
// Blink period (LED1) can be changed live from Serial Monitor

const int LED1 = 3;
const int LED2 = 4;
const int BTN  = 2;

unsigned long prevMillis = 0;
unsigned long period = 1000;  // default 1 second
bool led1State = false;

int led2State = LOW;
int lastBtn = HIGH;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("Type a number (ms) to change LED1 period:");
}

void loop() {
  unsigned long now = millis();

  // --- LED1 blink based on period ---
  if (now - prevMillis >= period) {
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

  // --- Check Serial input for new period ---
  if (Serial.available() > 0) {
    unsigned long newPeriod = Serial.parseInt();
    if (newPeriod > 0) {
      period = newPeriod;
      Serial.print("New blink period: ");
      Serial.print(period);
      Serial.println(" ms");
    }
    Serial.readStringUntil('\n');
  }
}
