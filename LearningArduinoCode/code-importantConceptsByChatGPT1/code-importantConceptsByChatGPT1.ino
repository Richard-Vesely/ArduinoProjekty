/****************************************************
 🧠 ARDUINO DATOVÉ TYPY A STRUKTURY — VÝUKOVÝ SOUBOR
 Každou sekci můžeš zkopírovat do nového souboru.
****************************************************/


/****************************************************
1️⃣  PROMĚNNÉ (int, float, char, bool)
-----------------------------------------------------
📘 NÁZEV: Základní datové typy
📍 POUŽITÍ: měření teploty, počítání bliknutí LED, rozhodování
****************************************************/

// int - celé číslo (např. počet bliknutí)
int pocet = 0;

// float - desetinné číslo (např. teplota, napětí)
float teplota = 23.7;

// char - jeden znak (např. 'A', 'B', 'C')
char pismeno = 'A';

// bool - pravda/nepravda (např. jestli je tlačítko stisknuté)
bool sviti = false;

void setup() {
  Serial.begin(9600);
  Serial.println("=== 1️⃣ PROMĚNNÉ ===");
  
  Serial.print("Počet: "); Serial.println(pocet);
  Serial.print("Teplota: "); Serial.println(teplota);
  Serial.print("Písmeno: "); Serial.println(pismeno);
  Serial.print("Svítí? "); Serial.println(sviti);
}

void loop() {}

/*
💡 Kde se to hodí:
- počítadlo skóre
- měření teploty z čidla
- ukládání písmen pro heslo
- logika: "když je něco pravda, rozviť LED"

❓ Kvíz:
1) Co se stane, když napíšeš "float pocet = 3 / 2;"?
   (Zkusíš, uvidíš, že výsledek není 1.5!)
*/


/****************************************************
2️⃣  POLE (arrays)
-----------------------------------------------------
📘 NÁZEV: Pole hodnot
📍 POUŽITÍ: RGB LED (3 barvy), průměrná teplota za týden, více tlačítek
****************************************************/

// Pole má více hodnot stejného typu
int hodnoty[5] = {10, 20, 30, 40, 50};

void setup2() {
  Serial.begin(9600);
  Serial.println("=== 2️⃣ POLE ===");

  for (int i = 0; i < 5; i++) {
    Serial.print("Hodnota ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(hodnoty[i]);
  }
}

/*
💡 Kde se to hodí:
- uložení 3 hodnot pro RGB LED (červená, zelená, modrá)
- ukládání posledních 10 měření ze senzoru
- práce s více motory nebo tlačítky

❓ Kvíz:
1) Co se stane, když zkusíš přistoupit na hodnoty[10]?
2) Jak bys změnil první hodnotu na 100?
*/


/****************************************************
3️⃣  ŘETĚZCE (String)
-----------------------------------------------------
📘 NÁZEV: Texty a zprávy
📍 POUŽITÍ: zobrazování na displeji, posílání textů do Serial monitoru
****************************************************/

String jmeno = "Arduino";
String pozdrav = "Ahoj, " + jmeno + "!";

void setup3() {
  Serial.begin(9600);
  Serial.println("=== 3️⃣ STRING ===");
  Serial.println(pozdrav);
}

/*
💡 Kde se to hodí:
- posílání zpráv přes Serial
- tisk na LCD displej
- ukládání jména, hesla, WiFi SSID

❓ Kvíz:
1) Co se stane, když přidáš do jmena: jmeno = jmeno + " Uno"; ?
2) Jak bys zjistil délku textu? (hint: jmeno.length())
*/


/****************************************************
4️⃣  STRUKTURA (struct)
-----------------------------------------------------
📘 NÁZEV: Vlastní typ — skupina hodnot
📍 POUŽITÍ: pozice robota (x, y, směr), údaje o senzoru
****************************************************/

struct Senzor {
  String nazev;
  float hodnota;
  String jednotka;
};

Senzor teplotni = {"Teplomer", 22.4, "°C"};

void setup4() {
  Serial.begin(9600);
  Serial.println("=== 4️⃣ STRUCT ===");
  Serial.print(teplotni.nazev);
  Serial.print(": ");
  Serial.print(teplotni.hodnota);
  Serial.println(teplotni.jednotka);
}

/*
💡 Kde se to hodí:
- více údajů o jednom senzoru (např. název + hodnota + jednotka)
- robot: pozice x, y a směr
- student: jméno, věk, skóre

❓ Kvíz:
1) Jak přidáš další senzor? 
   Senzor vlhkost = {"Vlhkomer", 60.0, "%"};
2) Co se stane, když změníš teplotni.hodnota = 30; ?
*/


/****************************************************
5️⃣  FUNKCE
-----------------------------------------------------
📘 NÁZEV: Opakovaně použitelný blok kódu
📍 POUŽITÍ: rozsvítit LED, spočítat průměr, reagovat na tlačítko
****************************************************/

int secti(int a, int b) {
  return a + b;
}

void setup5() {
  Serial.begin(9600);
  Serial.println("=== 5️⃣ FUNKCE ===");
  int soucet = secti(5, 7);
  Serial.print("Součet 5 + 7 = ");
  Serial.println(soucet);
}

/*
💡 Kde se to hodí:
- když potřebuješ stejný kód vícekrát (např. vypočítat PWM)
- kód je přehlednější
- pomáhá rozdělit úlohy (např. loop() je čistší)

❓ Kvíz:
1) Co se stane, když do funkce secti() pošleš float místo int?
*/


/****************************************************
6️⃣  PODMÍNKY (if / else)
-----------------------------------------------------
📘 NÁZEV: Rozhodování
📍 POUŽITÍ: zapnutí LED při stisku tlačítka, reagování na teplotu
****************************************************/

int teplota6 = 28;

void setup6() {
  Serial.begin(9600);
  Serial.println("=== 6️⃣ IF / ELSE ===");

  if (teplota6 > 30) {
    Serial.println("Je horko!");
  } else if (teplota6 < 10) {
    Serial.println("Je zima!");
  } else {
    Serial.println("Teplota je akorát.");
  }
}

/*
💡 Kde se to hodí:
- podmínky z čidel (např. jestli je světlo / tma)
- kontrola hodnot
- řízení motoru podle směru joysticku

❓ Kvíz:
1) Co se stane, když změníš teplota6 na 35?
2) Co když podmínky obrátíš (if < místo >)?
*/


/****************************************************
7️⃣  CYKLY (for / while)
-----------------------------------------------------
📘 NÁZEV: Opakování
📍 POUŽITÍ: blikání LED několikrát, postupné rozsvícení pixelů
****************************************************/

void setup7() {
  Serial.begin(9600);
  Serial.println("=== 7️⃣ FOR CYKLUS ===");

  for (int i = 1; i <= 5; i++) {
    Serial.print("Krok ");
    Serial.println(i);
    delay(500);
  }
}

/*
💡 Kde se to hodí:
- opakování akce (např. měření 10×)
- animace LED
- počítání iterací

❓ Kvíz:
1) Co se stane, když změníš i <= 5 na i < 5?
2) Co udělá "i += 2" místo "i++"?
*/


/****************************************************
8️⃣  ENUM (výčtový typ)
-----------------------------------------------------
📘 NÁZEV: Sady pojmenovaných stavů
📍 POUŽITÍ: řízení režimů robota, světelných efektů, stavů hry
****************************************************/

enum Rezim {STOP, DOPREDU, DOZADU};
Rezim aktualni = DOPREDU;

void setup8() {
  Serial.begin(9600);
  Serial.println("=== 8️⃣ ENUM ===");
  if (aktualni == DOPREDU) {
    Serial.println("Robot jede dopředu!");
  }
}

/*
💡 Kde se to hodí:
- přepínání režimů (STOP, JÍZDA, OTÁČENÍ)
- stavový automat
- jednoduché hry nebo efekty

❓ Kvíz:
1) Co se stane, když napíšeš "aktualni = STOP;"?
2) Co je výhoda oproti int proměnné s čísly 0,1,2?
*/


/****************************************************
9️⃣  KONSTANTY A #define
-----------------------------------------------------
📘 NÁZEV: Hodnoty, které se nemění
📍 POUŽITÍ: číslo pinu, rychlost motoru, délka pole
****************************************************/

#define LED_PIN 13
const int MAX_SPEED = 255;

void setup9() {
  Serial.begin(9600);
  Serial.println("=== 9️⃣ KONSTANTY ===");
  Serial.print("LED_PIN = "); Serial.println(LED_PIN);
  Serial.print("MAX_SPEED = "); Serial.println(MAX_SPEED);
}

/*
💡 Kde se to hodí:
- definice pinů a konstantních parametrů
- přehlednost kódu

❓ Kvíz:
1) Co se stane, když zkusíš změnit MAX_SPEED = 100; ?
2) Co se stane, když LED_PIN změníš na jiný pin?
*/


/****************************************************
🔟  ZÁVĚR
-----------------------------------------------------
Zkus si každý příklad zvlášť.
Změň pár hodnot, sleduj co se stane.
Když kód "rozbiješ", je to v pořádku — to je učení!
****************************************************/
