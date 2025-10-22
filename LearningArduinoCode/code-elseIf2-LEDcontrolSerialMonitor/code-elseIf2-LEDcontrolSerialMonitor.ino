/*
🧩 ÚKOLY:
-----------------------------------------------------------
1️⃣ Napiš do Serial Monitoru číslo 1, 2 nebo 3 → příslušná LED se přepne.
2️⃣ Napiš jiné písmeno nebo číslo → Arduino odpoví „ehm, ehm.. nevím co po mně chceš“.
3️⃣ Přidej možnost napsat '0', která všechny LED zhasne.
4️⃣ Přidej výpis, který po každé změně vypíše stav všech LED (např. "LED1: ON, LED2: OFF, LED3: ON").
5️⃣ Zkus změnit čísla pinů LED podle své desky a vysvětli, jak to Arduino ví.
-----------------------------------------------------------
*/

const int LED1 = 2;
const int LED2 = 3;
const int LED3 = 4;

int state1 = LOW;
int state2 = LOW;
int state3 = LOW;

void setup() {
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  Serial.begin(9600);
  Serial.println("Napiš 1, 2 nebo 3 a přepni LED. :)");
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();   // read one character

    if (input == '1') {
      state1 = !state1;
      digitalWrite(LED1, state1);
      Serial.println("LED 1 přepnuta!");
    } else if (input == '2') {
      state2 = !state2;
      digitalWrite(LED2, state2);
      Serial.println("LED 2 přepnuta!");
    } else if (input == '3') {
      state3 = !state3;
      digitalWrite(LED3, state3);
      Serial.println("LED 3 přepnuta!");
    } else {
      Serial.print("ehm, ehm.. nevím co po mně chceš (");
      Serial.print(input);
      Serial.println(" není 1, 2 ani 3)");
    }
  }
}
/*
🧩 ÚKOLY:
-----------------------------------------------------------
1️⃣ Napiš do Serial Monitoru číslo 1, 2 nebo 3 → příslušná LED se přepne.
2️⃣ Napiš jiné písmeno nebo číslo → Arduino odpoví „ehm, ehm.. nevím co po mně chceš“.
3️⃣ Přidej možnost napsat '0', která všechny LED zhasne.
4️⃣ Přidej výpis, který po každé změně vypíše stav všech LED (např. "LED1: ON, LED2: OFF, LED3: ON").
5️⃣ Vysvětli, jaký rozdíl je mezi if/else if a switch – který kód se ti čte lépe?
-----------------------------------------------------------
*/