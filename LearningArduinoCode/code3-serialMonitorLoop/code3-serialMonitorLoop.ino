void setup() {
  Serial.begin(9600);   // Start serial communication at 9600 bits per second
}

void loop() {
  Serial.println("Hello from Arduino!");  // Print a message
  delay(1000);                            // Wait 1 second
}
