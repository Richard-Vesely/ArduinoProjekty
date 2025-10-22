// 1️⃣ PWM pomocí analogWrite() – hardware PWM
// (funguje jen na pinech s ~, např. 3, 5, 6, 9, 10, 11 na Arduino UNO)

const int ledPin = 9;  // musí být pin s ~

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Postupné zesilování jasu
  for (int i = 0; i <= 255; i++) {
    analogWrite(ledPin, i);  // 0 = zhasnuto, 255 = plný jas
    delay(10);
  }

  // Postupné zeslabování jasu
  for (int i = 255; i >= 0; i--) {
    analogWrite(ledPin, i);
    delay(10);
  }
}
