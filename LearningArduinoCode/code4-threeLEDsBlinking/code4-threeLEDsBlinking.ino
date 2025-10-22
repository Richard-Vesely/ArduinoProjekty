// Blink 3 LEDs one after another

const int LED1 = 2;   // First LED on pin 2
const int LED2 = 3;   // Second LED on pin 3
const int LED3 = 4;   // Third LED on pin 4

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
}

void loop() {
  digitalWrite(LED1, HIGH);   // Turn on LED 1
  delay(1000);                // Wait 1 second
  digitalWrite(LED1, LOW);    

  digitalWrite(LED2, HIGH);   // Turn on LED 2
  delay(1000);                
  digitalWrite(LED2, LOW);

  digitalWrite(LED3, HIGH);   // Turn on LED 3
  delay(1000);                
  digitalWrite(LED3, LOW);
}
