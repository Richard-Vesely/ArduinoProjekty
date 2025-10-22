/****************************************************
✨ 3 LED BLINKER – EACH WITH DIFFERENT TIMING
-----------------------------------------------------
📘 GOAL:
Control 3 LEDs, each blinking with its own ON/OFF time.
No delay() used — everything runs at once using millis().

📍 USE CASES:
- Testing multiple indicators
- Learning about non-blocking code
- Building a traffic light or signal controller
****************************************************/

// 🔹 1. Define LED pins
const int LED1 = 3;
const int LED2 = 4;
const int LED3 = 5;

// 🔹 2. Define ON/OFF durations (in milliseconds)
unsigned long onTime1  = 200;   // LED1 ON for 200 ms
unsigned long offTime1 = 800;   // LED1 OFF for 800 ms

unsigned long onTime2  = 500;   // LED2 ON for 500 ms
unsigned long offTime2 = 500;   // LED2 OFF for 500 ms

unsigned long onTime3  = 1000;  // LED3 ON for 1000 ms
unsigned long offTime3 = 200;   // LED3 OFF for 200 ms

// 🔹 3. Variables to track LED states and last toggle times
bool ledState1 = false;
bool ledState2 = false;
bool ledState3 = false;

unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
unsigned long previousMillis3 = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  Serial.println("=== 3× LED BLINKER STARTED ===");
}

void loop() {
  unsigned long currentMillis = millis();  // current time

  // 🔹 LED1 — custom timing
  if (ledState1 && currentMillis - previousMillis1 >= onTime1) {
    ledState1 = false;
    previousMillis1 = currentMillis;
    digitalWrite(LED1, LOW);
  }
  else if (!ledState1 && currentMillis - previousMillis1 >= offTime1) {
    ledState1 = true;
    previousMillis1 = currentMillis;
    digitalWrite(LED1, HIGH);
  }

  // 🔹 LED2 — custom timing
  if (ledState2 && currentMillis - previousMillis2 >= onTime2) {
    ledState2 = false;
    previousMillis2 = currentMillis;
    digitalWrite(LED2, LOW);
  }
  else if (!ledState2 && currentMillis - previousMillis2 >= offTime2) {
    ledState2 = true;
    previousMillis2 = currentMillis;
    digitalWrite(LED2, HIGH);
  }

  // 🔹 LED3 — custom timing
  if (ledState3 && currentMillis - previousMillis3 >= onTime3) {
    ledState3 = false;
    previousMillis3 = currentMillis;
    digitalWrite(LED3, LOW);
  }
  else if (!ledState3 && currentMillis - previousMillis3 >= offTime3) {
    ledState3 = true;
    previousMillis3 = currentMillis;
    digitalWrite(LED3, HIGH);
  }
}
