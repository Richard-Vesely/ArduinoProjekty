/****************************************************
📘 ARDUINO — DATOVÉ TYPY A OPERACE (VÝUKOVÝ SOUBOR)
Každou sekci můžeš zkopírovat a spustit samostatně.
Autor: ChatGPT + Richard Veselý
****************************************************/


/****************************************************
1️⃣  INT — CELÁ ČÍSLA
-----------------------------------------------------
📍 POUŽITÍ: počítání bliknutí, vzdálenost v cm, skóre hry
****************************************************/

void setup() {
  Serial.begin(9600);
  Serial.println("=== 1️⃣ INT — CELÁ ČÍSLA ===");

  int a = 5;
  int b = 2;
  int soucet = a + b;      // sčítání
  int rozdil = a - b;      // odčítání
  int soucin = a * b;      // násobení
  int podil = a / b;       // dělení — POZOR: bez desetinné části
  int zbytek = a % b;      // zbytek po dělení (modulo)

  Serial.print("a = "); Serial.println(a);
  Serial.print("b = "); Serial.println(b);
  Serial.print("a + b = "); Serial.println(soucet);
  Serial.print("a / b = "); Serial.println(podil);
  Serial.print("a % b = "); Serial.println(zbytek);
}

/*
💡 Příklad použití:
- Počítání kolikrát blikla LED
- Měření vzdálenosti v centimetrech (celá čísla)
- Počítání skóre v hře

❓ Kvíz:
1) Co se stane, když napíšeš int podil = 5 / 2 ?
   (Odpověď: Výsledek je 2, ne 2.5)
2) Jak dostaneš přesnější výsledek?
*/


/****************************************************
2️⃣  FLOAT — DESETINNÁ ČÍSLA
-----------------------------------------------------
📍 POUŽITÍ: měření teploty, napětí, vzdálenosti s přesností
****************************************************/

void setup2() {
  Serial.begin(9600);
  Serial.println("=== 2️⃣ FLOAT — DESETINNÁ ČÍSLA ===");

  float x = 5.0;
  float y = 2.0;
  float vysledek = x / y;

  Serial.print("x / y = ");
  Serial.println(vysledek);   // vypíše 2.50

  float napeti = 4.98;
  float proud = 0.12;
  float vykon = napeti * proud;
  Serial.print("Výkon = "); Serial.println(vykon);  // 0.5976
}

/*
💡 Příklad použití:
- Výpočet fyzikálních veličin (napětí, výkon, rychlost)
- Měření teploty z čidla (např. 22.37 °C)
- Převody jednotek

❓ Kvíz:
1) Co se stane, když místo float použiješ int?
2) Co udělá Serial.println(3/2); ?
*/


/****************************************************
3️⃣  CHAR — JEDNOTLIVÉ ZNAKY
-----------------------------------------------------
📍 POUŽITÍ: ukládání písmen, znaků z klávesnice, menu
****************************************************/

void setup3() {
  Serial.begin(9600);
  Serial.println("=== 3️⃣ CHAR — ZNAKY ===");

  char pismeno = 'A';
  char dalsi = pismeno + 1;  // podle ASCII tabulky

  Serial.print("Původní písmeno: "); Serial.println(pismeno);
  Serial.print("Další písmeno: "); Serial.println(dalsi);

  // Můžeš také porovnávat znaky:
  bool jeVelke = (pismeno >= 'A' && pismeno <= 'Z');
  Serial.print("Je velké písmeno? "); Serial.println(jeVelke);
}

/*
💡 Příklad použití:
- Výběr menu (uživatel zmáčkne 'A', 'B', 'C')
- Uložení jednotlivých znaků do hesla
- Práce s ASCII tabulkou

❓ Kvíz:
1) Co se stane, když napíšeš 'A' + 2 ?
2) Co se stane, když použiješ dvojité uvozovky "A" místo jednoduchých 'A' ?
*/


/****************************************************
4️⃣  BOOL — PRAVDA / NEPRAVDA
-----------------------------------------------------
📍 POUŽITÍ: logika, rozhodování, stavy tlačítek nebo senzorů
****************************************************/

void setup4() {
  Serial.begin(9600);
  Serial.println("=== 4️⃣ BOOL — TRUE / FALSE ===");

  bool jeZapnuto = true;
  bool maBaterii = false;

  Serial.print("Je zapnuto? "); Serial.println(jeZapnuto);
  Serial.print("Má baterii? "); Serial.println(maBaterii);

  bool funguje = jeZapnuto && maBaterii;  // logické AND
  Serial.print("Funguje zařízení? "); Serial.println(funguje);
}

/*
💡 Příklad použití:
- Ověření, že všechno funguje (např. motor i senzor)
- Zapnutí/vypnutí LED
- Podmínky pro alarm

❓ Kvíz:
1) Co se stane, když použiješ || místo && ?
2) Co se stane, když bool proměnnou vytiskneš jako číslo?
*/


/****************************************************
5️⃣  BYTE — ČÍSLA 0–255 (1 bajt)
-----------------------------------------------------
📍 POUŽITÍ: LED barvy, PWM, síťová data
****************************************************/

void setup5() {
  Serial.begin(9600);
  Serial.println("=== 5️⃣ BYTE ===");

  byte red = 255;   // maximum
  byte green = 120;
  byte blue = 0;

  Serial.print("RGB: ");
  Serial.print(red); Serial.print(", ");
  Serial.print(green); Serial.print(", ");
  Serial.println(blue);

  // Operace s byty
  byte soucet = red / 2 + green / 2;
  Serial.print("Smíchaná barva: "); Serial.println(soucet);
}

/*
💡 Příklad použití:
- Nastavení barev pro RGB LED (0–255)
- Odesílání malých dat přes sériový port
- Paměťově úsporné počítání

❓ Kvíz:
1) Co se stane, když byte překročí 255?
2) Co je rozdíl mezi byte a int?
*/


/****************************************************
6️⃣  STRING — TEXT
-----------------------------------------------------
📍 POUŽITÍ: zprávy, jména, příkazy, komunikace přes Serial
****************************************************/

void setup6() {
  Serial.begin(9600);
  Serial.println("=== 6️⃣ STRING ===");

  String jmeno = "Arduino";
  String zprava = "Ahoj, " + jmeno + "!";
  Serial.println(zprava);

  int delka = jmeno.length();
  Serial.print("Délka jména: "); Serial.println(delka);

  char prvni = jmeno.charAt(0);
  Serial.print("První písmeno: "); Serial.println(prvni);

  jmeno.toLowerCase();
  Serial.print("Malými písmeny: "); Serial.println(jmeno);
}

/*
💡 Příklad použití:
- Posílání zpráv do Serial monitoru
- Jména WiFi sítí, hesla, příkazy
- Texty na LCD

❓ Kvíz:
1) Co se stane, když přidáš jmeno = jmeno + " Uno"; ?
2) Co udělá jmeno.toUpperCase(); ?
*/


/****************************************************
7️⃣  UNSIGNED INT / LONG — VĚTŠÍ ČÍSLA
-----------------------------------------------------
📍 POUŽITÍ: čas (millis), počítadla, časové intervaly
****************************************************/

void setup7() {
  Serial.begin(9600);
  Serial.println("=== 7️⃣ UNSIGNED INT / LONG ===");

  unsigned int velke = 40000;    // int by to nezvládl
  unsigned long cas = millis();  // systémový čas od spuštění
  Serial.print("Velké číslo: "); Serial.println(velke);
  Serial.print("Čas od spuštění (ms): "); Serial.println(cas);
}

/*
💡 Příklad použití:
- Počítání milisekund (millis vrací unsigned long)
- Časovače bez overflow
- Velká čísla (např. vzdálenost v mm)

❓ Kvíz:
1) Co se stane, když unsigned int = -5 ?
2) Co udělá millis() po 50 dnech nepřetržitého běhu?
*/


/****************************************************
/****************************************************
9️⃣  ARRAYS — POLE
-----------------------------------------------------
📘 NÁZEV: Pole hodnot (Array)
📍 POUŽITÍ: ukládání více čísel nebo hodnot stejného typu
   (např. RGB barvy, měření ze senzorů, více tlačítek)
****************************************************/

void setup9() {
  Serial.begin(9600);
  Serial.println("=== 9️⃣ ARRAYS — POLE ===");

  // 🔹 ZÁKLADNÍ DEKLARACE
  // Můžeme vytvořit pole s pevnou velikostí a hodnotami:
  int cisla[5] = {10, 20, 30, 40, 50};

  // Přístup k jednotlivým prvkům — indexy začínají od 0:
  Serial.print("První prvek: "); Serial.println(cisla[0]);  // 10
  Serial.print("Poslední prvek: "); Serial.println(cisla[4]); // 50

  // Změna hodnoty v poli:
  cisla[2] = 99;
  Serial.print("Třetí prvek po změně: "); Serial.println(cisla[2]);

  // Výpočet s prvky:
  int soucet = cisla[0] + cisla[1] + cisla[2];
  Serial.print("Součet prvních tří prvků: "); Serial.println(soucet);

  // 🔹 DEKLARACE PRÁZDNÉHO POLE
  // Můžeme vytvořit pole bez hodnot a později je přiřadit:
  int mereni[3];
  mereni[0] = 100;
  mereni[1] = 120;
  mereni[2] = 115;
  Serial.print("Druhé měření: "); Serial.println(mereni[1]);

  // 🔹 DÉLKA POLE
  int delka = sizeof(cisla) / sizeof(cisla[0]);
  Serial.print("Délka pole cisla: "); Serial.println(delka);

  // 🔹 POLE JINÝCH TYPŮ
  float teploty[4] = {22.5, 23.1, 22.8, 22.9};
  Serial.print("Průměrná teplota = ");
  float prumer = (teploty[0] + teploty[1] + teploty[2] + teploty[3]) / 4.0;
  Serial.println(prumer);

  // 🔹 POLE ZNAKŮ (text)
  char jmeno[6] = {'A', 'r', 'd', 'u', 'i', 'n'}; // pozor, bez '\0' není to String!
  Serial.print("První písmeno jména: "); Serial.println(jmeno[0]);
}

/*
💡 Kde se to hodí:
------------------
- Ukládání měření (např. 10 hodnot z teplotního senzoru)
- RGB LED: pole {R, G, B}
- Více tlačítek nebo LED (např. 4 LED = pole pinů)
- Seznam výsledků, skóre

📗 Vysvětlení:
--------------
Pole (array) je "řada přihrádek" se stejným typem dat.
Každá má své číslo (index). První má číslo 0.
Když napíšeš cisla[0], bereš první přihrádku.

🔍 Důležité:
------------
- Index začíná od 0 (poslední prvek = velikost - 1)
- Přístup mimo hranice pole (např. cisla[10]) může způsobit chybu
- sizeof(pole)/sizeof(pole[0]) = počet prvků v poli

❓ Kvíz:
-------
1) Co se stane, když zkusíš vytisknout cisla[5] ?
2) Jak bys změnil druhou hodnotu v poli cisla na 200 ?
3) Co se stane, když přepíšeš float teploty[0] = 30.5; ?
4) Co bys přidal, aby šlo spočítat průměr libovolně dlouhého pole automaticky?
*/


/****************************************************
🔟  BONUS: PRAKTICKÝ MINI PROJEKT — 3 LEDKY
-----------------------------------------------------
📘 Cíl: zapnout 3 LED postupně pomocí pole pinů
****************************************************/

void setup10() {
  Serial.begin(9600);
  Serial.println("=== 🔟 BONUS: 3 LEDKY ===");

  int ledky[3] = {3, 4, 5};   // LED na pinech 3, 4, 5

  // Nastavení všech pinů jako výstup
  pinMode(ledky[0], OUTPUT);
  pinMode(ledky[1], OUTPUT);
  pinMode(ledky[2], OUTPUT);

  // Zapni všechny postupně (bez smyčky pro jednoduchost)
  digitalWrite(ledky[0], HIGH);
  delay(300);
  digitalWrite(ledky[1], HIGH);
  delay(300);
  digitalWrite(ledky[2], HIGH);
  delay(300);

  Serial.println("Všechny LED zapnuté!");
}

/*
💡 Kde se to hodí:
------------------
- Místo 3 samostatných proměnných máš jedno pole.
- Snadno změníš počet LEDek bez velkého přepisování kódu.

❓ Kvíz:
-------
1) Co se stane, když přidáš čtvrtou LEDku na pin 6?
2) Jak bys LEDky vypnul v opačném pořadí?
*/


/****************************************************
🏁 ZÁVĚR — ARRAYS
-----------------------------------------------------
Pole jsou klíčem k tomu, aby kód byl přehledný a rozšiřitelný.
Zvládneš-li pole, zvládneš logiku pro více čidel, motorů i světel.
****************************************************/
/****************************************************
🔟  STRUCT — STRUKTURA
-----------------------------------------------------
📘 NÁZEV: Skupina různých dat dohromady
📍 POUŽITÍ: když chceš mít všechny údaje o jedné věci
   (např. senzor: jméno, hodnota, jednotka)
****************************************************/

// Nejprve definujeme "plán", jak bude struktura vypadat
struct Senzor {
  String nazev;
  float hodnota;
  String jednotka;
};

// Pak vytvoříme konkrétní proměnné typu Senzor
Senzor teplomer = {"Teplota", 22.7, "°C"};
Senzor vlhkomer = {"Vlhkost", 55.3, "%"};

void setup10() {
  Serial.begin(9600);
  Serial.println("=== 🔟 STRUCT — STRUKTURY ===");

  // Přístup k jednotlivým položkám
  Serial.print(teplomer.nazev);
  Serial.print(": ");
  Serial.print(teplomer.hodnota);
  Serial.println(teplomer.jednotka);

  Serial.print(vlhkomer.nazev);
  Serial.print(": ");
  Serial.print(vlhkomer.hodnota);
  Serial.println(vlhkomer.jednotka);

  // Můžeme hodnoty měnit
  teplomer.hodnota = 25.0;
  Serial.print("Nová teplota: ");
  Serial.println(teplomer.hodnota);

  // A dokonce vytvořit pole struktur!
  Senzor senzory[2] = {teplomer, vlhkomer};

  Serial.println("📋 Výpis všech senzorů:");
  Serial.print(senzory[0].nazev); Serial.print(" = "); Serial.println(senzory[0].hodnota);
  Serial.print(senzory[1].nazev); Serial.print(" = "); Serial.println(senzory[1].hodnota);
}

/*
💡 Kde se to hodí:
------------------
- Uložení údajů o senzoru: název + hodnota + jednotka
- Popis pozice robota: x, y, směr
- Hráč: jméno, skóre, životy
- Motor: pin1, pin2, rychlost, směr

📗 Vysvětlení:
--------------
Struct je jako „krabička“ s přihrádkami pro různé druhy dat.
Může obsahovat int, float, String, bool… prostě všechno.
Každý prvek se pak označuje tečkou:
  teplomer.hodnota, teplomer.nazev, teplomer.jednotka

🔍 Důležité:
------------
- Struktury jsou užitečné, když máš víc údajů o jedné věci.
- Pomáhají udržet pořádek v kódu.
- Můžeš jich mít více nebo i pole struktur.

❓ Kvíz:
-------
1) Jak bys přidal nový senzor „Tlak“ s hodnotou 1013 a jednotkou „hPa“?
2) Co se stane, když změníš teplomer.hodnota = 30; ?
3) Jak bys vytiskl všechny senzory z pole senzory[ ] ?
4) Může struktura obsahovat jinou strukturu? (Ano – zkus!)
*/


/****************************************************
🏁 BONUS: STRUKTURA ROBOTA
-----------------------------------------------------
📘 Příklad: Ukládáme pozici robota a jeho směr
****************************************************/

struct Robot {
  String jmeno;
  int x;
  int y;
  int smer;   // 0=nahoru, 1=vpravo, 2=dolu, 3=vlevo
};

Robot mujRobot = {"Wall-E", 0, 0, 1};

void setup11() {
  Serial.begin(9600);
  Serial.println("=== 🦾 ROBOT STRUCT ===");

  Serial.print("Jméno: "); Serial.println(mujRobot.jmeno);
  Serial.print("Pozice: ("); Serial.print(mujRobot.x); Serial.print(", ");
  Serial.print(mujRobot.y); Serial.println(")");
  Serial.print("Směr: "); Serial.println(mujRobot.smer);

  // Změna pozice (robot se pohne doprava)
  mujRobot.x += 5;
  mujRobot.smer = 2; // otočí se dolů
  Serial.println("Robot se pohnul a otočil:");
  Serial.print("Nová pozice: ("); Serial.print(mujRobot.x); Serial.print(", ");
  Serial.print(mujRobot.y); Serial.println(")");
  Serial.print("Směr: "); Serial.println(mujRobot.smer);
}

/*
💡 Kde se to hodí:
------------------
- Simulace pohybu robota
- Ukládání informací o herních objektech
- Stav robota: pozice, energie, režim, atd.

❓ Kvíz:
-------
1) Co se stane, když přidáš do struktury Robot nový prvek „energie“?
2) Jak bys vytvořil pole tří robotů?
3) Co znamená mujRobot.x += 5; ?
*/


/****************************************************
🏁 ZÁVĚR — STRUCT
-----------------------------------------------------
Struktury jsou základem pro vytváření „vlastních datových typů“.
Díky nim můžeš kód organizovat jako opravdový programátor:
přehledně, srozumitelně a s menší chybovostí.
****************************************************/
s

