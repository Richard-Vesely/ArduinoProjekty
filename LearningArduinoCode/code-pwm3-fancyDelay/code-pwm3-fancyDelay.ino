// 3️⃣ Ruční PWM s proměnnou délkou svitu (duty cycle)

const int ledPin = 4;
int duty = 20;       // počáteční jas (% z doby zapnutí)
int period = 10;     // délka jednoho cyklu v milisekundách

void setup() {
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Jeden cyklus PWM
  digitalWrite(ledPin, HIGH);
  delay(period * duty / 100);          // zapnuto po dobu duty%
  digitalWrite(ledPin, LOW);
  delay(period * (100 - duty) / 100);  // vypnuto po zbytek času

  // Plynulé zesilování a zeslabování jasu
  duty = duty + 5;
  if (duty > 100) duty = 0;
}
