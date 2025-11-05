// 3 LEDs blinking manually with delays
// LED1: every 1 second
// LED2: every 200 ms
// LED3: every 500 ms
// Total period: 2 seconds

const int LED1 = 4;
const int LED2 = 3;
const int LED3 = 2;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
}

void loop() {
  // time = 0 ms
  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  digitalWrite(LED3, HIGH);
  delay(200);

  // time = 200 ms
  digitalWrite(LED2, LOW);
  delay(200);

  // time = 400 ms
  digitalWrite(LED2, HIGH);
  delay(100);

  // time = 500 ms
  digitalWrite(LED3, LOW);
  delay(100);

  // time = 600 ms
  digitalWrite(LED2, LOW);
  delay(200);

  // time = 800 ms
  digitalWrite(LED2, HIGH);
  delay(200);

  // time = 1000 ms
  digitalWrite(LED1, LOW);   // LED1 completes its 1-second cycle
    digitalWrite(LED2, LOW);

  digitalWrite(LED3, HIGH);   

  delay(200);

  // time = 1200 ms
  digitalWrite(LED2, HIGH);
  delay(200);

  // time = 1400 ms
  digitalWrite(LED2, LOW);
  delay(100);

  // time = 1500 ms
    digitalWrite(LED3, LOW);  // LED3 turns back on (500 ms off)
  delay(100);

  // time = 1600 ms
  digitalWrite(LED2, HIGH);
  delay(200);

  // time = 1800 ms
  digitalWrite(LED2, LOW);
  delay(200);                // = 2000 ms total
}
