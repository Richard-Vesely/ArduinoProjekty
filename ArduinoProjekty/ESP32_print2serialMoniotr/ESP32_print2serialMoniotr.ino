// Minimal ESP32 sanity test (Serial only)
// Open Serial Monitor at 115200 baud after upload.

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\nESP32 alive. Hello!");
}

void loop() {
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    last = millis();
    Serial.printf("Uptime: %lu ms\n", (unsigned long)millis());
  }
}
