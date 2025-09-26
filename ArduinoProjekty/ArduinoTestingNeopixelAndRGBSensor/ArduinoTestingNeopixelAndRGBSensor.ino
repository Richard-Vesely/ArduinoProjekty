/*
🔌 Zapojení (česky, pro děti):
- NeoPixel kruh 8 LED (WS2812 / NeoPixel Ring 8):
  DIN → pin 6 na Arduinu
  VCC → 5V
  GND → GND
- Všechna GND propojit (Arduino ↔ LED kruh). 
- V Serial Monitoru nastav: Rychlost 9600 a Line ending: Newline.

💡 Tipy:
- Pokud máš Arduino Leonardo, po nahrání vyber správný Port (může se změnit).
- Pokud by data zlobila, dej mezi pin 6 a DIN rezistor ~330 Ω a velký kondenzátor 1000 µF mezi 5V a GND u LED.

🕹 Ovládání přes Serial Monitor:
- "on"  → zapne světelnou show
- "off" → vypne kroužek (zhasne)
- "b 0..255" → nastaví jas (např. "b 120")
- "speed 5..50" → rychlost animace v ms (menší = rychlejší)

📘 Úkoly, pro pochopení kódu:
1) Jaké nastavení musí mít Serial Monitor, aby příkazy fungovaly?
2) Kde v kódu se rozhoduje, jak rychle běží duha?
3) Jak změním jas kroužku?
4) Co udělám, aby místo duhy svítila pořád jen modře?
5) Proč je výhodné mít animaci „neblokující“ (bez velkých delay)?
*/

#include <Adafruit_NeoPixel.h>

#define PIXEL_PIN    6
#define NUMPIXELS    8
#define SERIAL_BAUD  9600

Adafruit_NeoPixel pixels(NUMPIXELS, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

bool ringOn = true;
uint8_t brightness = 100;      // 0–255
uint8_t j = 0;                 // fázový posun duhy
unsigned long lastStep = 0;
unsigned long stepInterval = 20; // ms; "speed X" příkazem změníš rychlost

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.begin(SERIAL_BAUD);
  // U Leonarda počkáme max 3 s na otevření Serial Monitoru (nezamrzne to):
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0 < 3000)) { ; }

  pixels.begin();
  pixels.setBrightness(brightness);
  pixels.clear();
  pixels.show();

  Serial.println(F("NeoPixel 8 ready. Příkazy: on, off, b <0-255>, speed <5-50>"));
  Serial.println(F("Nastav v Serial Monitoru 9600 baud a Line ending: Newline."));
}

void loop() {
  // Animace bez blokování – krok každých stepInterval ms
  if (ringOn && millis() - lastStep >= stepInterval) {
    lastStep = millis();
    rainbowStep();
  }

  // Příjem a zpracování příkazů
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();

    if (cmd.equalsIgnoreCase("on")) {
      ringOn = true;
      Serial.println(F("LED ring: ON"));
    } 
    else if (cmd.equalsIgnoreCase("off")) {
      ringOn = false;
      pixels.clear();
      pixels.show();
      Serial.println(F("LED ring: OFF"));
    }
    else if (cmd.startsWith("b ")) {       // jas
      int val = cmd.substring(2).toInt();
      val = constrain(val, 0, 255);
      brightness = (uint8_t)val;
      pixels.setBrightness(brightness);
      if (!ringOn) { pixels.show(); }      // aby se jas projevil i když je off (zůstane tma)
      Serial.print(F("Brightness: ")); Serial.println(brightness);
    }
    else if (cmd.startsWith("speed ")) {   // rychlost animace
      int ms = cmd.substring(6).toInt();
      ms = constrain(ms, 5, 50);
      stepInterval = (unsigned long)ms;
      Serial.print(F("Speed (ms/step): ")); Serial.println(stepInterval);
    }
    else {
      Serial.println(F("Neznamy prikaz. Pouzij: on, off, b <0-255>, speed <5-50>"));
    }
  }
}

// Jeden krok duhové animace (neblokující)
void rainbowStep() {
  for (uint16_t i = 0; i < pixels.numPixels(); i++) {
    pixels.setPixelColor(i, Wheel( (i * 256 / pixels.numPixels() + j) & 255 ));
  }
  pixels.show();
  j = (j + 1) & 0xFF;
}

// Převod čísla 0..255 na barvu (duhové kolečko)
uint32_t Wheel(byte p) {
  p = 255 - p;
  if (p < 85) {
    return pixels.Color(255 - p * 3, 0, p * 3);
  } else if (p < 170) {
    p -= 85;
    return pixels.Color(0, p * 3, 255 - p * 3);
  } else {
    p -= 170;
    return pixels.Color(p * 3, 255 - p * 3, 0);
  }
}
