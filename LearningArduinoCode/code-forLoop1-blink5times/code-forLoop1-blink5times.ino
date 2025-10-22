const int LED = 13;

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(9600);

  for (int i = 0; i < 5; i++) {
    digitalWrite(LED, HIGH);
    delay(200);
    digitalWrite(LED, LOW);
    delay(200);

    Serial.print("Blink number ");
    Serial.println(i + 1);
  }

  Serial.println("Done!");
}

void loop() {
  // empty
}
