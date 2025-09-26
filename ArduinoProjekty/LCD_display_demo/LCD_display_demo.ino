/*
===================== ZAPOJENÍ LCD2004 I2C (20x4 znaky) =====================

Arduino Leonardo  ->  LCD2004 I2C modul (PCF8574)
-------------------------------------------------
5V                ->  VCC   (napájení displeje)
GND               ->  GND   (společná zem)
D2                ->  SDA   (datová linka I2C)
D3                ->  SCL   (hodinová linka I2C)

Poznámky:
- SDA a SCL jsou I2C linky = vodiče, po kterých spolu zařízení komunikují.
- Fungují jako "open-drain": zařízení umí linku stáhnout na 0, ale neumí ji tlačit na 1.
- Proto na modulu jsou pull-up rezistory (~4.7k), které linku drží v log. 1.
- Modul PCF8574 převádí I2C signál na paralelní řídicí piny LCD displeje.
=============================================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C adresa: nejčastěji 0x27 nebo 0x3F
LiquidCrystal_I2C lcd(0x27, 20, 4); // (adresa, znaky na řádku, počet řádků)

void setup() {
  lcd.init();            // inicializace displeje
  lcd.backlight();       // zapnout podsvícení

  lcd.setCursor(0,0);    // řádek 0, sloupec 0
  lcd.print("Hello, Arduino!");

  lcd.setCursor(0,1);
  lcd.print("I2C LCD 20x4");

  lcd.setCursor(0,2);
  lcd.print("Open-drain I2C :)");

  lcd.setCursor(0,3);
  lcd.print("Line 4 text here");
}

void loop() {
  lcd.scrollDisplayLeft(); // posun textu
  delay(800);
}
