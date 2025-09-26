/************************************************************
ZAPOJENÍ (pro děti – stručně a jasně)

Arduino Leonardo  →  LCD 20×4 s I2C (modul s čipem PCF8574)
------------------------------------------------------------
5V   → VCC   (napájení displeje)
GND  → GND   (společná zem – propojit!)
D2   → SDA   (datová linka I2C)
D3   → SCL   (hodinová linka I2C)

Tipy:
- Na modulu bývá I2C adresa nejčastěji 0x27 (někdy 0x3F).
- Když nic nevidíš: otoč potenciometr kontrastu na modulu (malý šroubek).
- I2C umí víc zařízení na stejných dvou vodičích (SDA, SCL) – super pro projekty!

ÚKOLY, PRO POCHOPENÍ KÓDU (CHALLENGES)
1) Napište něco na třetí a čtvrtý řádek displeje.

2) Napište:
   - Na začátek prvního řádku: „Ahoj“
   - Do PŮLKY druhého řádku: libovolný text
   - Na KONEC posledního řádku: „johA“
   Ukázka, jak by to MĚLO vypadat (pseudograficky):
   [řádek 1] Ahoj____________________
   [řádek 2] ________TVŮJ_TEXT_______
   [řádek 3] ________________________
   [řádek 4] ________________johA____
   (Podtržítka jen znázorňují prázdná místa.)

3.1) Napište někam do KÓDU slovo "pricnezna" tak, aby SE ZOBRAZILO na displeji.

3.2) Napište někam do KÓDU slovo "princezna" tak, aby SE NEZOBRAZILO na displeji.

4.1) ODkomentujte v kódu řádek tak, aby se text na obrazovce POSOUVAL DOLEVA.
4.2) Upravte kód tak, aby se text posouval NAOPAK – DOPRAVA.

5) Naprogramuj LCD displej tak, aby se text „jsi moc fajn“ zobrazoval postupně od prvního řádku až po čtvrtý. Na každém řádku se má držet jednu sekundu, pak zmizet a objevit se o řádek níž.
Příklad (časová osa):
0 s → „jsi moc fajn“ na 1. řádku
1 s → „jsi moc fajn“ na 2. řádku
2 s → „jsi moc fajn“ na 3. řádku
3 s → „jsi moc fajn“ na 4. řádku
4 s → znovu od 1. řádku …

BONUS nápady:
- Zkus vypsat čísla, smajlíky, nebo vytvořit vlastní znak (Custom Character).

OTÁZKY K ZAMYŠLENÍ
1. Co znamená `#include` na začátku kódu?  
2. Proč musíme použít knihovnu `LiquidCrystal_I2C.h`?  
3. Co dělá příkaz `lcd.init();` a proč ho potřebujeme v `setup()`?  
4. Jaký je rozdíl mezi `lcd.setCursor(x, y)` a `lcd.print("text")`?  
5. Co dělá `lcd.clear()` a proč se hodí, když chci text „pohybovat“?  
6. Jak funguje příkaz `delay(1000);`? Co by se stalo, kdyby tam bylo `delay(200);`?  
7. Jak bys změnil kód, aby se text pohyboval nahoru místo dolů?  
8. Co by se stalo, kdybychom místo `lcd.clear()` použili jen další `lcd.print()`?  


************************************************************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ZMĚŇ, POKUD JE POTŘEBA: nejčastěji 0x27 (někdy 0x3F)
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Přepínač posouvání (použij v Úkolu 4.1/4.2)
bool scrollLeft = true;   // nastav na false pro posun doprava (Úkol 4.2)

void setup() {
  lcd.init();
  lcd.backlight();

  // ==== A) Jen DVA příkazy, které opravdu něco vypíšou ====
  lcd.setCursor(0, 0); 
  lcd.print("Hello, LCD 20x4!");  // 1. skutečné vypsání

  lcd.setCursor(0, 1);
  lcd.print("Zkus moje U K O L Y"); // 2. skutečné vypsání

  
}

void loop() {

  // lcd.scrollDisplayLeft();
  // delay(400);

  delay(20);
}
