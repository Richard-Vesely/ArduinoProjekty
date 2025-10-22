// 2️⃣ Ruční PWM – pevné časy ON a OFF
// LED se zapíná a vypíná ručně, ale výsledek vypadá jako ztlumené světlo

const int ledPin = 3;

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  digitalWrite(ledPin, HIGH);  // LED ON
  delay(2);                    // zapnuto krátce
  digitalWrite(ledPin, LOW);   // LED OFF
  delay(8);                    // vypnuto déle
  // Cyklus trvá 10 ms -> 20 % svítí, 80 % zhasnuto = slabý jas
}
