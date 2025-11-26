/******************************************************
🪛 Zapojení (pro děti – jak to propojit):
Joystick má 5 pinů: **GND, +5V (nebo 3V3), VRx, VRy, SW**

Zapoj to takto:
- GND → GND na ESP32  
- +5V (nebo 3V3) → 3V3 na ESP32  
- VRx → pin 34  (budeme mu říkat **Left**)  
- VRy → pin 35  (budeme z něj počítat **Right = Y-**)  
- SW → pin 32 (nepovinné tlačítko)

*******************************************************
🧩 Úkoly pro pochopení kódu:
1️⃣ Co dělá funkce `analogRead()`?  
2️⃣ Proč používáme `>> 4` místo funkce `map()`?  
3️⃣ Proč se osa Y obrací pomocí `255 - ...`?  
4️⃣ Jak bys přidal tlačítko SW, aby se tisklo jen při stisku?
*******************************************************/

#define VRX_PIN 35   // X osa = Left
#define VRY_PIN 34   // Y osa = Right (Y-)
#define SW_PIN  32   // tlačítko (nepovinné)

void setup() {
  Serial.begin(115200);
  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
}

void loop() {
  // Načteme surové hodnoty 0–4095
  int rawX = analogRead(VRX_PIN);
  int rawY = analogRead(VRY_PIN);

  // Převedeme na 0–255 (rychlý převod)
  int left  = rawX >> 4;
  int right = 255 - (rawY >> 4);  // Y- obrácená osa

  Serial.print("Left: ");
  Serial.print(left);
  Serial.print("  Right: ");
  Serial.println(right);

  // žádné delay() — běží nepřetržitě bez čekání
}
