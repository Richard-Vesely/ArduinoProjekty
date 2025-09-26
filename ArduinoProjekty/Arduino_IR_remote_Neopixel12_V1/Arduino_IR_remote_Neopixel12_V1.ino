/******************************************************
ZAPOJENÍ (Arduino Leonardo + NeoPixel 12 + IR přijímač)
-------------------------------------------------------
NeoPixel 12 kruh (WS2812/NeoPixel):
- DIN (data)  -> D6 přes odpor ~330 Ω
- 5V          -> 5V (nebo externích 5V; GND společná!)
- GND         -> GND
(Doporučeno: elektrolytický kondenzátor 470–1000 µF mezi 5V a GND u pásku.)

IR přijímač (např. VS1838B, TSOP38238…):
- OUT         -> D2  (IR_RECEIVE_PIN)
- VCC         -> 5V
- GND         -> GND

POZNÁMKY:
- Pokud používáš externí 5V pro LED, PROPOJ GND externího zdroje s GND Arduina.
- Napájení z USB zvládne 12 NeoPixelů pro jemné efekty, ale pro jistotu preferuj externí 5V.

OVLÁDÁNÍ
- Tlačítko "1"  -> Show 1: Duha (plynulý rainbow-cyklus)
- Tlačítko "2"  -> Show 2: Theater chase (běžící světla)
- Tlačítko "3"  -> Show 3: Dýchání barvy (plynulá změna jasu)
- Opakování (NEC repeat 0x0) se ignoruje nebo recykluje poslední stisk.

Úkoly, pro pochopení kódu (zkus odpovědět):
1) Proč používáme millis() místo delay()? Co by se rozbilo s delay() při čtení IR?
2) Kde se v kódu řeší NEC repeat (0x0) a proč to potřebujeme?
3) Jak snadno přidáš Show 4? Kde nastavíš přepínání show a kde krokování animace?
4) Proč srovnáváme proti konstantám z myIRcodes.h a nikdy nepíšeme „0x…“ do if/switch?
5) Co se stane, když nepropojíš společnou zem mezi Arduinem, IR přijímačem a LED páskem?
******************************************************/

#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp>        // IRremote v3+
#include <MyIRcodes.h>         // Tvoje header-only knihovna s pojmenovanými kódy (ONE, TWO, THREE, ...)
using namespace MyIR;          // přinese ONE, TWO, THREE přímo do scope
// =================== UPRAV PODLE POTŘEBY ===================
#define LED_PIN         6          // Data pin pro NeoPixel kruh
#define NUMPIXELS       12         // NeoPixel 12 kruh
#define IR_RECEIVE_PIN  2          // Datový pin z IR přijímače
#define LED_BRIGHTNESS  120        // 0–255 (globální max jas)
// ===========================================================

Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Stavové proměnné pro výběr show
uint8_t currentShow = 1;      // 1..3
unsigned long lastStepMs = 0; // čas posledního kroku animace
unsigned long stepDelayMs = 30; // defaultní rychlost (mění se podle show)
uint16_t animStep = 0;        // krok animace (inkrementujeme v čase)

// IR – uchování posledního nerepeat kódu pro práci s NEC repeat (0x0)
unsigned long lastNonRepeat = 0;

// ---------- Pomocné: převod barvy „kolečko“ ----------
uint32_t wheel(byte pos) {
  pos = 255 - pos;
  if (pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  } else if (pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  } else {
    pos -= 170;
    return strip.Color(pos * 3, 255 - pos * 3, 0);
  }
}

// ---------- Show 1: Rainbow cycle (plynulá duha) ----------
void showRainbowStep() {
  // rychlost cyklu
  stepDelayMs = 20;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    // posuneme kolečko podle animStep
    strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + animStep) & 0xFF));
  }
  strip.show();
  animStep++;
}

// ---------- Show 2: Theater Chase (běžící světla) ----------
void showTheaterChaseStep() {
  stepDelayMs = 120;
  // střídavý vzor – tři fáze (offset 0,1,2)
  uint8_t offset = animStep % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i % 3 == offset) {
      strip.setPixelColor(i, strip.Color(255, 180, 40)); // teplejší bílá/žlutá
    } else {
      strip.setPixelColor(i, 0); // vypnuto
    }
  }
  strip.show();
  animStep++;
}

// ---------- Show 3: Dýchání (plynulý nádech-výdech jasu jedné barvy) ----------
void showBreathingStep() {
  stepDelayMs = 15;
  // sinusové dýchání – mapa 0..255 -> ~0..1
  // použijeme „trojúhelníkovou“ aproximaci bez trigonometrie:
  // 0..255..0 (pilovitě do trojúhelníku)
  uint8_t phase = animStep & 0xFF;
  uint8_t tri = phase < 128 ? phase * 2 : (255 - phase) * 2; // 0..255..0
  // zvol barvu – např. azurová
  uint8_t r = 0;
  uint8_t g = (uint16_t)tri * 2 / 3;   // tlumenější zelená
  uint8_t b = tri;                      // modrá
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
  animStep++;
}

// ---------- Zobraz jméno tlačítka pro Serial debug ----------
const char* buttonName(unsigned long code) {
  // Porovnáváme proti symbolům z myIRcodes.h (např. ONE, TWO, THREE, ...).
  // Přidej si sem další case podle své knihovny:
  if (code == ONE)   return "ONE";
  if (code == TWO)   return "TWO";
  if (code == THREE) return "THREE";
  // doplň si klidně FOUR, FIVE, ... podle své lib.
  return "UNKNOWN";
}

// ---------- Zpracování přijatého IR signálu ----------
void handleIR() {
  if (!IrReceiver.decode()) return;

  unsigned long raw = IrReceiver.decodedIRData.decodedRawData;

  // Debug výstup: raw kód + jméno tlačítka (pokud známe)
  Serial.print(F("IR raw=0x"));
  Serial.print(raw, HEX);
  if (raw == 0x0 && lastNonRepeat != 0) {
    Serial.print(F(" (REPEAT of "));
    Serial.print(buttonName(lastNonRepeat));
    Serial.print(F(")"));
  } else {
    Serial.print(F(" ("));
    Serial.print(buttonName(raw));
    Serial.print(F(")"));
  }
  Serial.println();

  // NEC repeat (0x0) – použij poslední platný kód
  unsigned long signal = (raw == 0x0 && lastNonRepeat != 0) ? lastNonRepeat : raw;

  // Přepínání show podle tvých pojmenovaných konstant
  if (signal == ONE) {
    currentShow = 1;
    animStep = 0;
    Serial.println(F(">> SHOW 1: Rainbow"));
  } else if (signal == TWO) {
    currentShow = 2;
    animStep = 0;
    Serial.println(F(">> SHOW 2: Theater chase"));
  } else if (signal == THREE) {
    currentShow = 3;
    animStep = 0;
    Serial.println(F(">> SHOW 3: Breathing"));
  }

  // Ulož poslední nerepeat
  if (raw != 0x0) {
    lastNonRepeat = raw;
  }

  IrReceiver.resume(); // Připravit přijímač na další data
}

// ---------- Arduino životní cyklus ----------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println(F("\nIR + NeoPixel controller start"));

  // NeoPixel
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.show(); // zhasnout

  // IRremote
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // vestavěná LEDka pro odezvu
  // Volitelné: IrReceiver.setTolerance(25); // v IRremote v3 není vždy dostupné; nech zakomentované

  Serial.println(F("IR přijímač připraven. Stiskni 1 / 2 / 3."));
}

void loop() {
  // 1) IR obsluha
  handleIR();

  // 2) Ne-blokující animace podle currentShow
  unsigned long now = millis();
  if (now - lastStepMs >= stepDelayMs) {
    lastStepMs = now;
    switch (currentShow) {
      case 1: showRainbowStep();     break;
      case 2: showTheaterChaseStep();break;
      case 3: showBreathingStep();   break;
      default: currentShow = 1;      break;
    }
  }
}
