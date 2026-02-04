/******************************************************
ZAPOJENÍ (ESP32 + L298N + 2× DC motor: Motor A i Motor B)
-------------------------------------------------------
ESP32          ->  L298N
-----------------------------
GPIO 18 (IN1)  ->  IN1   (směr Motor A)
GPIO 19 (IN2)  ->  IN2   (směr Motor A)
GPIO 23 (ENA)  ->  ENA   (rychlost Motor A — PWM)

GPIO 5  (IN3)  ->  IN3   (směr Motor B)
GPIO 17 (IN4)  ->  IN4   (směr Motor B)
GPIO 22 (ENB)  ->  ENB   (rychlost Motor B — PWM)

5V (nebo VIN)  ->  5V    (napájení L298N)
GND            ->  GND   (SPOLEČNÁ ZEM s ESP32!)

Motory:
- Motor A připoj na svorky OUT1 a OUT2.
- Motor B připoj na svorky OUT3 a OUT4.

DŮLEŽITÉ:
- ENA a ENB na modulu NESMÍ být překlenuty jumpery (jinak PWM z ESP32 nepůjde).
- Propoj GND ESP32 a GND L298N.
- USB z ESP32 často motor neutáhne — použij zvlášť zdroj 6–12 V pro motory (GND společná).

OVLÁDÁNÍ PŘES SERIAL MONITOR (115200 baud):
- Zadej dvě čísla v rozsahu -255 až 255 oddělená mezerou/čárkou:  např.  "200 -120"
    - první = Motor A (levé kolo), druhé = Motor B (pravé kolo)
- Jedno číslo bez oddělení (např. "150") nastaví OBOU motorům stejnou rychlost.
- Záporné = dozadu, kladné = dopředu, 0 = stop.

-------------------------------------------------------
ÚKOLY PRO POCHOPENÍ KÓDU:
1) Proč nastavujeme směr zvlášť přes IN1/IN2 a rychlost přes ENA/ENB?
2) Co přesně dělá část kódu, kde jsou oba směrové piny LOW? (Jak se motor chová?)
3) Uprav kód tak, aby rozsah byl -100 až 100. Kde to změníš?
4) Přidej „mrtvou zónu“: hodnoty od -30 do 30 ber jako 0 (kvůli tření).
******************************************************/

// --- Piny Motor A (levé kolo) ---
#define IN1 18
#define IN2 19
#define ENA 23   // PWM

// --- Piny Motor B (pravé kolo) ---
#define IN3 5
#define IN4 17
#define ENB 22   // PWM

// Pomocné funkce: nastavení směru a rychlosti (-255..255)
void setMotorA(int value) {
  value = constrain(value, -255, 255);
  if (value > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, value);        // rychlost vpřed
  } else if (value < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, -value);       // rychlost vzad (kladná PWM)
  } else {
    // stop (volnoběh)
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
  }
}

void setMotorB(int value) {
  value = constrain(value, -255, 255);
  if (value > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, value);
  } else if (value < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, -value);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
  }
}

// Jednoduché parsování dvou čísel z řádku (oddělené mezerou nebo čárkou).
// Pokud uživatel pošle jen jedno číslo, použije se pro oba motory.
bool parseTwoInts(const String &line, int &a, int &b, bool &onlyOne) {
  String s = line;
  s.trim();
  if (s.length() == 0) return false;

  // Najdi oddělovač (mezera nebo čárka)
  int sep = s.indexOf(' ');
  if (sep < 0) sep = s.indexOf(',');
  if (sep < 0) {
    // pouze jedno číslo
    a = s.toInt();
    onlyOne = true;
    return true;
  } else {
    String sA = s.substring(0, sep);
    String sB = s.substring(sep + 1);
    sA.trim(); sB.trim();
    if (sA.length() == 0 && sB.length() == 0) return false;
    if (sA.length() == 0) sA = "0";
    if (sB.length() == 0) sB = "0";
    a = sA.toInt();
    b = sB.toInt();
    onlyOne = false;
    return true;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Bezpečné zastavení na startu
  setMotorA(0);
  setMotorB(0);

  Serial.println("Dvojmotorovy test (ESP32 + L298N)");
  Serial.println("Zadej -255..255 pro A a B (napr. '200 -120'), nebo jedno cislo pro oba (napr. '150').");
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    int a = 0, b = 0;
    bool onlyOne = false;

    if (parseTwoInts(line, a, b, onlyOne)) {
      a = constrain(a, -255, 255);
      if (onlyOne) b = a; else b = constrain(b, -255, 255);

      setMotorA(a);
      setMotorB(b);

      Serial.print("Motor A = ");
      Serial.print(a);
      Serial.print(" | Motor B = ");
      Serial.println(b);
    }

    // Vyčisti případné zbytky v bufferu
    while (Serial.available()) Serial.read();
  }
}

/*
POZNÁMKY:
- Na ESP32 může být potřeba knihovna analogWrite. Pokud by překladač hlásil,
  že analogWrite neexistuje, přidej na úplný začátek:
    #include <analogWrite.h>
- PWM hodnota 0..255 určuje „střední“ napětí na ENA/ENB => rychlost motoru.
- Máš-li trhavý rozjezd, zkus menší hodnoty (např. 120) nebo přidej rampu.
*/
