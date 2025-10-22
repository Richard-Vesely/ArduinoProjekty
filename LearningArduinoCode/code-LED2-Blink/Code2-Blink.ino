// Blink an LED using a variable for the pin number

const int LED_PIN = 13;  // Define the LED pin number

void setup() {
  pinMode(LED_PIN, OUTPUT);  // Set the LED pin as output
}

void loop() {
  digitalWrite(LED_PIN, HIGH);  // Turn LED on
  delay(1000);                  // Wait 1 second
  digitalWrite(LED_PIN, LOW);   // Turn LED off
  delay(1000);                  // Wait 1 second
}
