/******************************************************
ZAPOJENÍ (Arduino Leonardo + NeoPixel 12 + IR přijímač)
-------------------------------------------------------
NeoPixel 12 kruh (WS2812/WS2812B/NeoPixel):
- DIN (data)  -> D6 přes odpor ~330 Ω
- 5V          -> 5V (nebo externích 5V; GND společná!)
- GND         -> GND
(Doporučeno: elektrolytický kondenzátor 470–1000 µF mezi 5V a GND u LED.)

IR přijímač (VS1838B/TSOP38238 apod.):
- OUT         -> D2  (IR_RECEIVE_PIN)
- VCC         -> 5V
- GND         -> GND

POZNÁMKY:
/******************************************************
ÚKOLY PRO STUDENTY (RGB + porozumění kódu a IR ovladači)
=======================================================

CÍL:
- Pochopíš, jak fungují barvy RGB (R, G, B v rozsahu 0–255).
- Naučíš se číst a měnit kód tak, aby LED dělaly přesně to, co chceš.
- Každá „show“ má jeden konkrétní úkol. Část úkolů dělej bez pomoci ChatGPT.

⚠️ Tip k RGB:
https://www.csfieldguide.org.nz/en/interactives/rgb-mixer/

-------------------------------------------------------
Předchozí otázky k zamyšlení (nech v kódu):
1) Proč používáme millis() místo delay() pro animace a IR čtení?
2) Kde a jak se obsluhuje NEC repeat (0x0) a proč recyklujeme poslední nerepeat kód?
3) Jak jednoduše přidáš 10. show? (Jakou funkci přidat a kde přepínat?)
4) Proč porovnáváme IR kódy výhradně s konstantami (ONE, TWO, …) a ne s „0x…“?
5) Co se stane, když není společná GND mezi Arduinem, IR a LED?

-------------------------------------------------------
ÚKOLY KE KAŽDÉ SHOW (1 úkol = 1 show)

SHOW 1 — Rainbow Cycle
A) Zpomal nebo zrychli duhu úpravou proměnné stepDelayMs tak, aby přechody byly zřetelnější. 
B) *Navíc:* Změň „wheel()“ tak, aby duha měla jen 3 jasné pásy (R, G, B).

SHOW 2 — Theater Chase
A) Změň barvu běžících LED na RŮŽOVOU (bez pomoci ChatGPT). 
   Najdi řádek se strip.Color(255, 180, 40) a přepiš ho na růžovou.
B) *Navíc:* Uprav rozestup (mod 3) na mod 4 a vysvětli, co se vizuálně změní.

SHOW 3 — Breathing (dýchání)
A) Změň barvu dýchání z azurové na teplou bílou a uprav amplitudu, aby „nádech–výdech“ byl jemnější.
B) *Navíc:* Dýchej mezi dvěma barvami — při minimu tyrkys, při maximu fialová.

SHOW 4 — Comet (kometa)
A) Změň barvu hlavy komety na sytě modrou a ocas na slabě modrý (menší jas).
B) *Navíc:* Přidej druhou kometu, která běží proti směru první.

SHOW 5 — Color Wipe Cycle
A) Nahraď paletu „basicPalette“ vlastní paletou pěti barev podle tvého výběru.
B) *Navíc:* Uprav kód tak, aby se vždy po vyplnění celého kruhu jas krátce zvýšil a pak vrátil.

SHOW 6 — Twinkle (jiskření)
A) Změň barvu jisker ze „studené bílé“ na zlaté jiskry (víc červené a zelené, méně modré).
B) *Navíc:* Zaveď „rušivé zhasnutí“: občas náhodou zhasni všechny LED na 1 krok (blik).

SHOW 7 — Larson Scanner
A) Uprav barvu bodu na červenou a „ocáskům“ dej postupně klesající jas (např. 160, 80).
B) *Navíc:* Zvětši délku ocasu na 4 LED a udělej plynulejší fade.

SHOW 8 — Rain (kapky)
A) Změň barvu kapek na světle modrou a prodluž jejich „život“ (proměnná life).
B) *Navíc:* Přidej gravitační zrychlení: kapky se „rozjíždí“ (zkracuj jejich krokový interval).

SHOW 9 — Palette Pulse
A) Změň barvy v „palette9“ tak, aby odpovídaly tvému oblíbenému týmu/klubu.
B) *Navíc:* Uprav frekvenci přepínání palety (podíl s animStep), aby se barva měnila dvakrát rychleji.

-------------------------------------------------------
VYTVOŘ SI VLASTNÍ FUNKCI (DOPORUČENO S CHATGPT)

1) TheaterChaseCustomColor
   - Napiš novou funkci:  void theaterChaseCustomColor(uint8_t r, uint8_t g, uint8_t b, uint8_t gap)
   - Chování: „běžící tečky“ v barvě (r,g,b) s nastavitelným rozestupem „gap“.
   - Zařiď, aby se spustila jako nová SHOW 10 po stisku tlačítka „0“ (ZERO).

2) BreathingTwoColors
   - Funkce dýchá mezi dvěma barvami (r1,g1,b1) → (r2,g2,b2).
   - Parametr „speed“ ovlivní, jak rychle se barvy střídají.

3) RainBowComet
   - Kometa používá „wheel()“ pro hlavu (duhová) a ocasu plynule klesá jas.

Poznámka: Při přidání nové show nezapomeň:
- Přidat funkci (např. show10_theaterChaseCustomColor()).
- V „loop()“ doplnit do switch/case novou větev.
- Ošetřit volbu v handleIR() pro příslušné tlačítko.

-------------------------------------------------------
KRÁTKÝ SEBE-TEST (BEZ KÓDOVÁNÍ)
- Jak uděláš RŮŽOVOU, když máš jen RGB hodnoty? (odpověz trojicí čísel)
- Co znamená „jas“ u NeoPixelů a jak ho globálně ovlivníš v tomto kódu?
- Co přesně dělá proměnná „animStep“ napříč různými show?

HODNOCENÍ:
- Základ: splněno A-úkol u všech 9 show.
- Pokročilé: k alespoň 3 show máš i B-úkol.
- Extra: vytvořil(a) jsi alespoň 1 novou funkci a přidal(a) SHOW 10.

******************************************************/


#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp>   // IRremote v3+
#if !defined(ONE)
// IR button codes (formerly from MyIRcodes.h)
constexpr uint32_t ONE      = 0xBA45FF00;
constexpr uint32_t TWO      = 0xB946FF00;
constexpr uint32_t THREE    = 0xB847FF00;
constexpr uint32_t FOUR     = 0xBB44FF00;
constexpr uint32_t FIVE     = 0xBF40FF00;
constexpr uint32_t SIX      = 0xBC43FF00;
constexpr uint32_t SEVEN    = 0xF807FF00;
constexpr uint32_t EIGHT    = 0xEA15FF00;
constexpr uint32_t NINE     = 0xF609FF00;
constexpr uint32_t STARBTN  = 0xE916FF00;
constexpr uint32_t ZERO     = 0xE619FF00;
constexpr uint32_t HASHBTN  = 0xF20DFF00;
constexpr uint32_t UPBTN    = 0xE718FF00;
constexpr uint32_t LEFTBTN  = 0xF708FF00;
constexpr uint32_t OKBTN    = 0xE31CFF00;
constexpr uint32_t RIGHTBTN = 0xA55AFF00;
constexpr uint32_t DOWNBTN  = 0xAD52FF00;
#endif

// =================== UPRAV PODLE POTŘEBY ===================
#define LED_PIN         6
#define NUMPIXELS       12
#define IR_RECEIVE_PIN  2

// Limity a výchozí hodnoty
#define BRIGHT_MIN      5      // minimální jas (0–255), necháme >0, ať je něco vidět
#define BRIGHT_MAX      255
#define BRIGHT_STEP     10

#define SPEED_MIN_MS    5      // nejrychlejší krok (ms)
#define SPEED_MAX_MS    500    // nejpomalejší krok (ms)
#define SPEED_STEP_MS   10     // změna rychlosti na stisk

uint8_t  currentShow   = 1;                  // 1..9
uint8_t  globalBright  = 120;                // 0..255 (clamp v kódu)
uint16_t animStep      = 0;                  // krok animace
unsigned long lastStepMs  = 0;
unsigned long stepDelayMs = 30;              // rychlost animace (nižší = rychlejší)

Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// NEC repeat podpora
unsigned long lastNonRepeat = 0;

// ---------- Pomocné funkce ----------
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

void clampBrightness() {
  if (globalBright < BRIGHT_MIN) globalBright = BRIGHT_MIN;
  if (globalBright > BRIGHT_MAX) globalBright = BRIGHT_MAX;
  strip.setBrightness(globalBright);
}

void clampSpeed() {
  if (stepDelayMs < SPEED_MIN_MS) stepDelayMs = SPEED_MIN_MS;
  if (stepDelayMs > SPEED_MAX_MS) stepDelayMs = SPEED_MAX_MS;
}

void clearStrip() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
}

// ---------- Show 1: Rainbow Cycle (plynulá duha po kruhu) ----------
void show1_rainbowCycle() {
  // rychlá, plynulá
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + animStep) & 0xFF));
  }
  strip.show();
  animStep++;
}

// ---------- Show 2: Theater Chase (běžící trojice) ----------
void show2_theaterChase() {
  uint8_t offset = animStep % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i % 3 == offset) strip.setPixelColor(i, strip.Color(255, 180, 40));
    else strip.setPixelColor(i, 0);
  }
  strip.show();
  animStep++;
}

// ---------- Show 3: Breathing (dýchání azurové) ----------
void show3_breathing() {
  uint8_t phase = animStep & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2; // 0..255..0
  uint8_t r = 0, g = (uint16_t)tri * 2 / 3, b = tri;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
  animStep++;
}

// ---------- Show 4: Comet (svítící hlava + doznívající ocas) ----------
uint8_t cometPos = 0;
uint8_t cometTrail[NUMPIXELS]; // 0..255 jako „dosvit“

void show4_comet() {
  // lehké tlumení osvitů (fade)
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (cometTrail[i] > 5) cometTrail[i] -= 5;
    else cometTrail[i] = 0;
    // barva z osvitů: oranžovo-červená
    uint8_t v = cometTrail[i];
    strip.setPixelColor(i, strip.Color(v, (v * 2) / 3, 10));
  }
  // hlava komety
  cometTrail[cometPos] = 255;
  strip.show();

  cometPos = (cometPos + 1) % NUMPIXELS;
  animStep++;
}

// ---------- Show 5: Color Wipe Cycle (postupné zaplňování barvami) ----------
uint8_t wipeIndex = 0;
uint8_t wipeColorIndex = 0; // 0..5 (6 barev)

uint32_t basicPalette(uint8_t idx) {
  switch (idx % 6) {
    case 0: return strip.Color(255, 0, 0);
    case 1: return strip.Color(0, 255, 0);
    case 2: return strip.Color(0, 0, 255);
    case 3: return strip.Color(255, 255, 0);
    case 4: return strip.Color(255, 0, 255);
    default:return strip.Color(0, 255, 255);
  }
}

void show5_colorWipeCycle() {
  // postupně přebarvuje po jedné LED na aktuální barvu
  strip.setPixelColor(wipeIndex, basicPalette(wipeColorIndex));
  strip.show();
  wipeIndex++;
  if (wipeIndex >= NUMPIXELS) {
    wipeIndex = 0;
    wipeColorIndex++;
  }
  animStep++;
}

// ---------- Show 6: Twinkle (náhodné jiskření s pozvolným dohasínáním) ----------
uint8_t twinkleVal[NUMPIXELS]; // 0..255

void show6_twinkle() {
  // náhodně rozsvítit pár LED
  if (random(0, 100) < 30) {
    uint16_t p = random(0, NUMPIXELS);
    twinkleVal[p] = 180 + random(0, 76); // 180..255
  }
  // vyhasínání a kreslení
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (twinkleVal[i] > 3) twinkleVal[i] -= 3;
    else twinkleVal[i] = 0;
    uint8_t v = twinkleVal[i];
    // studená bílá jiskra
    strip.setPixelColor(i, strip.Color(v, v, (uint16_t)v * 3 / 2));
  }
  strip.show();
  animStep++;
}

// ---------- Show 7: Larson Scanner (Cylon) ----------
int8_t scanPos = 0;
int8_t scanDir = 1;

void show7_scanner() {
  clearStrip();
  // hlavní bod
  strip.setPixelColor(scanPos, strip.Color(255, 30, 30));
  // jemné „ocásky“
  int16_t left = scanPos - 1;
  int16_t right = scanPos + 1;
  if (left >= 0)  strip.setPixelColor(left,  strip.Color(120, 10, 10));
  if (right < NUMPIXELS) strip.setPixelColor(right, strip.Color(120, 10, 10));
  strip.show();

  scanPos += scanDir;
  if (scanPos <= 0 || scanPos >= (int)NUMPIXELS - 1) scanDir = -scanDir;
  animStep++;
}

// ---------- Show 8: Rain (náhodné „kapky“ běhající dokola) ----------
struct Drop { int8_t pos; uint8_t life; };
const uint8_t MAX_DROPS = 4;
Drop drops[MAX_DROPS];

void spawnDrop() {
  for (uint8_t i = 0; i < MAX_DROPS; i++) {
    if (drops[i].life == 0) {
      drops[i].pos = random(0, NUMPIXELS);
      drops[i].life = 30 + random(0, 40); // délka života
      return;
    }
  }
}

void show8_rain() {
  if (random(0, 100) < 25) spawnDrop();

  // tlumení pozadí
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8)  & 0xFF;
    uint8_t b = c & 0xFF;
    // fade
    if (r > 5) r -= 5; else r = 0;
    if (g > 5) g -= 5; else g = 0;
    if (b > 5) b -= 5; else b = 0;
    strip.setPixelColor(i, r, g, b);
  }

  // posun a vykreslení kapek
  for (uint8_t i = 0; i < MAX_DROPS; i++) {
    if (drops[i].life) {
      // barva kapky modrofialová
      strip.setPixelColor(drops[i].pos, strip.Color(40, 0, 200));
      drops[i].pos = (drops[i].pos + 1) % NUMPIXELS; // „padá“ dokola
      drops[i].life--;
    }
  }
  strip.show();
  animStep++;
}

// ---------- Show 9: Palette Pulse (pulsování přes paletu) ----------
const uint8_t PAL_SIZE = 5;
uint32_t palette9[PAL_SIZE] = {
  0xFF5500, // oranž
  0x00FF88, // zelenomodrá
  0x3355FF, // modrá
  0xFF00AA, // magenta
  0xFFFF22  // žlutá
};
uint8_t palIdx = 0;

void show9_palettePulse() {
  // trojúhelníková obálka jasu
  uint8_t phase = animStep & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint32_t base = palette9[palIdx];
  uint8_t br = tri; // 0..255

  uint8_t r = ((base >> 16) & 0xFF);
  uint8_t g = ((base >> 8)  & 0xFF);
  uint8_t b = (base & 0xFF);

  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    // jemný fázový posun po kruhu
    uint8_t local = (br * (uint8_t)((i * 8) % 64 + 192)) / 255;
    strip.setPixelColor(i, (uint16_t)r * local / 255, (uint16_t)g * local / 255, (uint16_t)b * local / 255);
  }
  strip.show();

  // každých ~1,5 cyklu přepneme barvu
  if ((animStep % 180) == 0) palIdx = (palIdx + 1) % PAL_SIZE;
  animStep++;
}

// ---------- Pomocný výpis názvu tlačítka (pro debug) ----------
const char* buttonName(unsigned long code) {
  if (code == ONE)   return "ONE";
  if (code == TWO)   return "TWO";
  if (code == THREE) return "THREE";
  if (code == FOUR)  return "FOUR";
  if (code == FIVE)  return "FIVE";
  if (code == SIX)   return "SIX";
  if (code == SEVEN) return "SEVEN";
  if (code == EIGHT) return "EIGHT";
  if (code == NINE)  return "NINE";
  if (code == LEFTBTN)  return "LEFT";
  if (code == RIGHTBTN) return "RIGHT";
  if (code == UPBTN)    return "UP";
  if (code == DOWNBTN)  return "DOWN";
  return "UNKNOWN";
}

// ---------- IR obsluha ----------
void handleIR() {
  if (!IrReceiver.decode()) return;

  unsigned long raw = IrReceiver.decodedIRData.decodedRawData;
  unsigned long signal = (raw == 0x0 && lastNonRepeat != 0) ? lastNonRepeat : raw;

  // Debug
  Serial.print(F("IR raw=0x")); Serial.print(raw, HEX);
  if (raw == 0x0 && lastNonRepeat) {
    Serial.print(F(" (REPEAT of ")); Serial.print(buttonName(lastNonRepeat)); Serial.print(F(")"));
  } else {
    Serial.print(F(" (")); Serial.print(buttonName(raw)); Serial.print(F(")"));
  }
  Serial.println();

  // Volba show 1..9
  if (signal == ONE)    { currentShow = 1;  animStep = 0; Serial.println(F(">> SHOW 1: Rainbow Cycle")); }
  else if (signal == TWO)   { currentShow = 2;  animStep = 0; Serial.println(F(">> SHOW 2: Theater Chase")); }
  else if (signal == THREE) { currentShow = 3;  animStep = 0; Serial.println(F(">> SHOW 3: Breathing")); }
  else if (signal == FOUR)  { currentShow = 4;  animStep = 0; memset(cometTrail,0,sizeof(cometTrail)); cometPos=0; Serial.println(F(">> SHOW 4: Comet")); }
  else if (signal == FIVE)  { currentShow = 5;  animStep = 0; wipeIndex=0; wipeColorIndex=0; Serial.println(F(">> SHOW 5: Color Wipe Cycle")); }
  else if (signal == SIX)   { currentShow = 6;  animStep = 0; memset(twinkleVal,0,sizeof(twinkleVal)); Serial.println(F(">> SHOW 6: Twinkle")); }
  else if (signal == SEVEN) { currentShow = 7;  animStep = 0; scanPos=0; scanDir=1; Serial.println(F(">> SHOW 7: Scanner")); }
  else if (signal == EIGHT) { currentShow = 8;  animStep = 0; for (uint8_t i=0;i<MAX_DROPS;i++){drops[i].life=0;} Serial.println(F(">> SHOW 8: Rain")); }
  else if (signal == NINE)  { currentShow = 9;  animStep = 0; palIdx=0; Serial.println(F(">> SHOW 9: Palette Pulse")); }

  // RYCHLOST: LEFT / RIGHT
  else if (signal == LEFTBTN) {
    if (stepDelayMs + SPEED_STEP_MS <= SPEED_MAX_MS) stepDelayMs += SPEED_STEP_MS;
    clampSpeed();
    Serial.print(F("<< SPEED slower: ")); Serial.print(stepDelayMs); Serial.println(F(" ms/step"));
  } else if (signal == RIGHTBTN) {
    if (stepDelayMs >= SPEED_STEP_MS) stepDelayMs -= SPEED_STEP_MS;
    clampSpeed();
    Serial.print(F(">> SPEED faster: ")); Serial.print(stepDelayMs); Serial.println(F(" ms/step"));
  }

  // JAS: UP / DOWN
  else if (signal == UPBTN) {
    if (globalBright + BRIGHT_STEP <= BRIGHT_MAX) globalBright += BRIGHT_STEP;
    else globalBright = BRIGHT_MAX;
    clampBrightness();
    Serial.print(F(">> BRIGHTNESS: ")); Serial.println(globalBright);
  } else if (signal == DOWNBTN) {
    if (globalBright >= BRIGHT_STEP + BRIGHT_MIN) globalBright -= BRIGHT_STEP;
    else globalBright = BRIGHT_MIN;
    clampBrightness();
    Serial.print(F("<< BRIGHTNESS: ")); Serial.println(globalBright);
  }

  // Ulož poslední nerepeat
  if (raw != 0x0) lastNonRepeat = raw;

  IrReceiver.resume(); // připravit přijímač na další data
}

// ---------- Setup & Loop ----------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println(F("\nIR + NeoPixel 12 — 9 shows, speed & brightness control"));

  strip.begin();
  strip.setBrightness(globalBright);
  strip.show();

  // pro náhodné efekty
  randomSeed(analogRead(A0));

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  // POZOR: některé verze IRremote už nemají setTolerance(); proto ho nepoužíváme.
  Serial.println(F("IR ready. Press 1–9 for shows, LEFT/RIGHT speed, UP/DOWN brightness."));
  clampSpeed();
  clampBrightness();
}

void loop() {
  handleIR();

  unsigned long now = millis();
  if (now - lastStepMs >= stepDelayMs) {
    lastStepMs = now;
    switch (currentShow) {
      case 1: show1_rainbowCycle();   break;
      case 2: show2_theaterChase();   break;
      case 3: show3_breathing();      break;
      case 4: show4_comet();          break;
      case 5: show5_colorWipeCycle(); break;
      case 6: show6_twinkle();        break;
      case 7: show7_scanner();        break;
      case 8: show8_rain();           break;
      case 9: show9_palettePulse();   break;
      default: currentShow = 1;       break;
    }
  }
}
