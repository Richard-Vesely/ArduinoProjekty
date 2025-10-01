/******************************************************
VERZE B — S „TRANSLATED HEXES“
(Option B: pevně dané konstanty v tomto souboru)

================ ZAPOJENÍ (Arduino Leonardo + NeoPixel 12 + IR přijímač) ================
NeoPixel 12 kruh (WS2812/WS2812B/NeoPixel):
- DIN (data)  -> D6 přes odpor ~330 Ω
- 5V          -> 5V (nebo externích 5V; GND společná!)
- GND         -> GND
(Doporučeno: elektrolytický kondenzátor 470–1000 µF mezi 5V a GND u LED.)

IR přijímač (VS1838B/TSOP38238 apod.):
- OUT         -> D2  (IR_RECEIVE_PIN)
- VCC         -> 5V
- GND         -> GND

Poznámky:
- Pokud napájíš LED externím 5V zdrojem, PROPOJ jeho GND s GND Arduina.
- 12 LED většinou zvládne USB, ale pro jistotu preferuj externí 5V (společná zem!).

OVLÁDÁNÍ IR OVLADAČEM
- Číslice 1–9: volba show (9 různých efektů).
- LEFT / RIGHT: zpomalit / zrychlit animaci.
- UP / DOWN: snížit / zvýšit jas.

Úkoly, pro pochopení kódu (zkus odpovědět):
1) Proč používáme millis() místo delay() pro animace a IR čtení?
2) Kde a jak se obsluhuje NEC repeat (0x0) a proč recyklujeme poslední nerepeat kód?
3) Jak jednoduše přidáš 10. show? (Jakou funkci přidat a kde přepínat?)
4) Proč porovnáváme IR kódy výhradně s konstantami (a ne „0x…“ přímo v if)?
5) Co se stane, když není společná GND mezi Arduinem, IR a LED? Jak se to projeví?
******************************************************/

#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp>   // IRremote v3+

// ============ Translated hexes ============
// (Nepoužívej „surové“ hex hodnoty v if/switch. Vždy porovnávej proti těmto konstantám.)
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

#define LED_PIN         6
#define NUMPIXELS       12
#define IR_RECEIVE_PIN  2

// Limity a výchozí hodnoty
#define BRIGHT_MIN      5
#define BRIGHT_MAX      255
#define BRIGHT_STEP     10

#define SPEED_MIN_MS    5
#define SPEED_MAX_MS    500
#define SPEED_STEP_MS   10

uint8_t  currentShow   = 1;     // 1..9
uint8_t  globalBright  = 120;   // 0..255
uint16_t animStep      = 0;
unsigned long lastStepMs  = 0;
unsigned long stepDelayMs = 30;

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

// ---------- Show 1: Rainbow Cycle ----------
void show1_rainbowCycle() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + animStep) & 0xFF));
  }
  strip.show();
  animStep++;
}

// ---------- Show 2: Theater Chase ----------
void show2_theaterChase() {
  uint8_t offset = animStep % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i % 3 == offset) strip.setPixelColor(i, strip.Color(255, 180, 40));
    else strip.setPixelColor(i, 0);
  }
  strip.show();
  animStep++;
}

// ---------- Show 3: Breathing ----------
void show3_breathing() {
  uint8_t phase = animStep & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint8_t r = 0, g = (uint16_t)tri * 2 / 3, b = tri;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
  animStep++;
}

// ---------- Show 4: Comet ----------
uint8_t cometPos = 0;
uint8_t cometTrail[NUMPIXELS]; // 0..255

void show4_comet() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (cometTrail[i] > 5) cometTrail[i] -= 5;
    else cometTrail[i] = 0;
    uint8_t v = cometTrail[i];
    strip.setPixelColor(i, strip.Color(v, (v * 2) / 3, 10));
  }
  cometTrail[cometPos] = 255;
  strip.show();

  cometPos = (cometPos + 1) % NUMPIXELS;
  animStep++;
}

// ---------- Show 5: Color Wipe Cycle ----------
uint8_t wipeIndex = 0;
uint8_t wipeColorIndex = 0;

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
  strip.setPixelColor(wipeIndex, basicPalette(wipeColorIndex));
  strip.show();
  wipeIndex++;
  if (wipeIndex >= NUMPIXELS) {
    wipeIndex = 0;
    wipeColorIndex++;
  }
  animStep++;
}

// ---------- Show 6: Twinkle ----------
uint8_t twinkleVal[NUMPIXELS];

void show6_twinkle() {
  if (random(0, 100) < 30) {
    uint16_t p = random(0, NUMPIXELS);
    twinkleVal[p] = 180 + random(0, 76);
  }
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (twinkleVal[i] > 3) twinkleVal[i] -= 3;
    else twinkleVal[i] = 0;
    uint8_t v = twinkleVal[i];
    strip.setPixelColor(i, strip.Color(v, v, (uint16_t)v * 3 / 2));
  }
  strip.show();
  animStep++;
}

// ---------- Show 7: Larson Scanner ----------
int8_t scanPos = 0;
int8_t scanDir = 1;

void show7_scanner() {
  clearStrip();
  strip.setPixelColor(scanPos, strip.Color(255, 30, 30));
  int16_t left = scanPos - 1;
  int16_t right = scanPos + 1;
  if (left >= 0)  strip.setPixelColor(left,  strip.Color(120, 10, 10));
  if (right < NUMPIXELS) strip.setPixelColor(right, strip.Color(120, 10, 10));
  strip.show();

  scanPos += scanDir;
  if (scanPos <= 0 || scanPos >= (int)NUMPIXELS - 1) scanDir = -scanDir;
  animStep++;
}

// ---------- Show 8: Rain ----------
struct Drop { int8_t pos; uint8_t life; };
const uint8_t MAX_DROPS = 4;
Drop drops[MAX_DROPS];

void spawnDrop() {
  for (uint8_t i = 0; i < MAX_DROPS; i++) {
    if (drops[i].life == 0) {
      drops[i].pos = random(0, NUMPIXELS);
      drops[i].life = 30 + random(0, 40);
      return;
    }
  }
}

void show8_rain() {
  if (random(0, 100) < 25) spawnDrop();

  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8)  & 0xFF;
    uint8_t b = c & 0xFF;
    if (r > 5) r -= 5; else r = 0;
    if (g > 5) g -= 5; else g = 0;
    if (b > 5) b -= 5; else b = 0;
    strip.setPixelColor(i, r, g, b);
  }

  for (uint8_t i = 0; i < MAX_DROPS; i++) {
    if (drops[i].life) {
      strip.setPixelColor(drops[i].pos, strip.Color(40, 0, 200));
      drops[i].pos = (drops[i].pos + 1) % NUMPIXELS;
      drops[i].life--;
    }
  }
  strip.show();
  animStep++;
}

// ---------- Show 9: Palette Pulse ----------
const uint8_t PAL_SIZE = 5;
uint32_t palette9[PAL_SIZE] = {
  0xFF5500, 0x00FF88, 0x3355FF, 0xFF00AA, 0xFFFF22
};
uint8_t palIdx = 0;

void show9_palettePulse() {
  uint8_t phase = animStep & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint32_t base = palette9[palIdx];
  uint8_t br = tri;

  uint8_t r = ((base >> 16) & 0xFF);
  uint8_t g = ((base >> 8)  & 0xFF);
  uint8_t b = (base & 0xFF);

  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint8_t local = (br * (uint8_t)((i * 8) % 64 + 192)) / 255;
    strip.setPixelColor(i, (uint16_t)r * local / 255, (uint16_t)g * local / 255, (uint16_t)b * local / 255);
  }
  strip.show();

  if ((animStep % 180) == 0) palIdx = (palIdx + 1) % PAL_SIZE;
  animStep++;
}

// ---------- Pomocný výpis názvu tlačítka ----------
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

  if (raw != 0x0) lastNonRepeat = raw;
  IrReceiver.resume();
}

// ---------- Setup & Loop ----------
void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println(F("\nIR + NeoPixel 12 — 9 shows, speed & brightness control (translated hexes)"));

  strip.begin();
  strip.setBrightness(globalBright);
  strip.show();

  randomSeed(analogRead(A0));

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
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
