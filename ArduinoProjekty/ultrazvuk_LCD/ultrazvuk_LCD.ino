#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD setup (20x4, I2C address 0x27 or 0x3F)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Ultrasonic pins
#define TRIG_PIN 9
#define ECHO_PIN 10

void setup() {
  // LCD init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Ultrasonic Demo");

  // Ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  delay(1000);
  lcd.clear();
}

long readDistanceCM() {
  // trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // measure echo pulse length
  long duration = pulseIn(ECHO_PIN, HIGH, 30000UL); // timeout 30ms (~5m)
  if (duration == 0) return -1; // no echo

  // convert to cm (speed of sound = 343 m/s)
  long distance = duration / 58; // common formula
  return distance;
}

void loop() {
  long d = readDistanceCM();

  lcd.setCursor(0,0);
  lcd.print("Distance:        "); // clear line
  lcd.setCursor(10,0);

  if (d < 0) {
    lcd.print("No echo");
  } else {
    lcd.print(d);
    lcd.print(" cm");
  }

  delay(200);
}
