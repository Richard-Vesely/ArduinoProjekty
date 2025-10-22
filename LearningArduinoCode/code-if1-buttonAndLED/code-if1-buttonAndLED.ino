// When the button is pressed, the LED turns on for 1 second

const int buttonPin = 2;  // Button connected to pin 2
const int ledPin = 3;     // LED connected to pin 3

void setup() {
  pinMode(buttonPin, INPUT_PULLUP); // internal pull-up resistor (button to GND)
  pinMode(ledPin, OUTPUT);
}

void loop() {
  int buttonState = digitalRead(buttonPin);

  if (buttonState == LOW) {          // button pressed
    digitalWrite(ledPin, HIGH);      // LED on
    delay(1000);                     // keep it on for 1 second
    digitalWrite(ledPin, LOW);       // then turn it off
  }
}
