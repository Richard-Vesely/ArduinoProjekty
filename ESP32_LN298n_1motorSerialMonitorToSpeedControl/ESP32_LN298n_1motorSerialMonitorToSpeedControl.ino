/******************************************************
ZAPOJENÍ (ESP32 + L298N + DC motor s ENA)
-------------------------------------------------------
ESP32          ->  L298N
-----------------------------
GPIO 18 (IN1)  ->  IN1
GPIO 19 (IN2)  ->  IN2
GPIO 23 (ENA)  ->  ENA   (PWM pin pro rychlost)
5V (nebo VIN)  ->  5V    (napájení L298N)
GND            ->  GND   (společná zem!)

DC MOTOR A připoj na svorky OUT1 a OUT2 na modulu L298N.

POZOR:
- ENA jumper musí být ODPOJENÝ (aby šel řídit z ESP32)
- GND ESP32 a L298N musí být propojené!
-------------------------------------------------------

ÚKOLY PRO POCHOPENÍ KÓDU:
1️⃣ Co znamená kladné a záporné číslo, které pošleš do Serial monitoru?  
2️⃣ Co dělá `abs()` a proč ho používáme u hodnoty rychlosti?  
3️⃣ Jak bys upravil kód, aby rozsah byl −100 až 100 místo −255 až 255?  
4️⃣ Jak bys přidal druhý motor (Motor B)?

******************************************************/

#define IN1 18
#define IN2 19
#define ENA 23

int speedValue = 0;

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  Serial.begin(115200);
  Serial.println("Zadej číslo od -255 do 255 pro nastavení rychlosti motoru:");
}

void loop() {
  if (Serial.available() > 0) {
    int input = Serial.parseInt();  // načti celé číslo
    if (input != 0 || Serial.peek() == '0') {
      speedValue = constrain(input, -255, 255);
      setMotorSpeed(speedValue);
      Serial.print("Rychlost nastavena na: ");
      Serial.println(speedValue);
    }
    // vyčistí buffer, aby se další číslo správně načetlo
    while (Serial.available()) Serial.read();
  }
}

void setMotorSpeed(int value) {
  if (value > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, value);
  } 
  else if (value < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, abs(value));
  } 
  else {  // stop
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
  }
}
