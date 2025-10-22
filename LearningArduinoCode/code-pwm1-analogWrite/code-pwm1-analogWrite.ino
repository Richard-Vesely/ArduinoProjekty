// PWM – LED svítí stálým jasem pomocí analogWrite()
// (funguje jen na pinech s ~, např. 3, 5, 6, 9, 10, 11 na Arduino UNO)

const int ledPin = 9;   // vyber pin s ~

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  analogWrite(ledPin, 128);  // jas 0–255 (128 = přibližně poloviční)
}
