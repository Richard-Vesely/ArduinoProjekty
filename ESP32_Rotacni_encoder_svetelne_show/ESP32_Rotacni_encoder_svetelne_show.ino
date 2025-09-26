/*
==================== ZAPOJENÍ (ESP32) ====================

ESP32           ->  NeoPixel 8 (WS2812/WS2812B)
---------------------------------------------------------
3V3/5V*         ->  +5V (VIN) LED   (*doporučeno externích 5V)
GND             ->  GND LED  (společná zem s ESP32 i PC)
GPIO18          ->  DIN LED  (doporučeno 330–470 Ω v sérii)
Kondenzátor 470–1000 µF mezi +5V a GND u LED (správná polarita)

ESP32           ->  Rotační enkodér (mechanický)
---------------------------------------------------------
GND             ->  GND
GPIO16          ->  CLK / A  (INPUT_PULLUP)
GPIO17          ->  DT  / B  (INPUT_PULLUP)
GPIO27          ->  SW (tlačítko; INPUT_PULLUP)

Poznámky:
- ESP32 je 3.3 V. WS2812 obvykle přijme 3.3 V datový signál, ale nejspolehlivější je:
  (a) LED napájet ~4.3–4.7 V NEBO (b) použít level shifter (74AHCT125/125N apod.).
- Vyhýbej se vstup-only pinům (34–39) pro výstupy; DIN LED na GPIO18 je bezpečná volba.
- Tlačítko na GPIO27 je “boot-safe” (nemaří start, na rozdíl od některých strapping pinů).
==========================================================
SERIÁLOVÝ PROTOKOL (115200 b/s) – STEJNÝ JAKO DŘÍV:
Příkazy z webu → ESP32:
- "MODE X"   ... X = 0..3
- "BRI N"    ... N = 0..255

Události z ESP32 → web:
- "EV UP", "EV DOWN", "EV PRESS", "READY"
==========================================================
*/

#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

#define LED_PIN    18
#define LED_COUNT  60
#define DEFAULT_BRIGHTNESS 40

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------- ENCODER ----------
const uint8_t PIN_CLK = 16;  // A / CLK
const uint8_t PIN_DT  = 17;  // B / DT
const uint8_t PIN_SW  = 27;  // tlačítko (na GND)

// Kolik kvadraturních kroků tvoří jeden “cvak” (detent) enkodéru.
const int8_t ENC_SCALE = 2;  // 2 nebo 4 podle typu enkodéru

volatile int16_t encAccum = 0;
Bounce btn;

// Volitelně obrátit směr
const bool ENC_INVERT = false;

volatile int8_t  encDelta = 0;   // změny z ISR
volatile uint8_t lastAB   = 0;

// Tabulka pro kvadraturní dekódování (Gray)
// index = (oldAB<<2) | newAB, hodnota = -1, 0, +1
int8_t qdecTable[16] = {
  0, -1, +1,  0,
  +1, 0,  0, -1,
  -1, 0,  0, +1,
   0, +1, -1, 0
};

inline uint8_t readAB() {
  // INPUT_PULLUP: sepnutí = LOW -> negujeme, abychom dostali “logickou” 1/0
  uint8_t a = !digitalRead(PIN_CLK);
  uint8_t b = !digitalRead(PIN_DT);
  return (a << 1) | b;
}

void IRAM_ATTR updateEncoder() {
  uint8_t newAB = readAB();
  uint8_t idx = (lastAB << 2) | newAB;
  encDelta += qdecTable[idx];
  lastAB = newAB;
}

// ---------- STAV EFEKTŮ ----------
uint8_t currentMode = 0;     // 0..9
uint8_t brightness  = DEFAULT_BRIGHTNESS;
uint8_t speedPercent = 50;   // 1..100 (vyšší = rychlejší)
unsigned long tNow;

struct RainbowState { uint16_t j = 0; } rainbow;
struct TheaterState { uint8_t offset = 0; } theater;
struct BounceState  { int pos = 0; int dir = 1; } bounce;
struct SparkleState { } sparkle;
struct WipeState    { uint16_t idx = 0; uint32_t color = 0; uint8_t phase = 0; bool clearing = true; } wipe;
struct TwinkleState { } twinkle;
struct CometState   { int pos = 0; int dir = 1; } comet;
struct FireState    { } fireFx;
struct BreatheState { uint16_t t = 0; } breathe;
struct ScannerState { int pos = 0; int dir = 1; } scanner;

// ---------- HELPERY ----------
uint32_t wheel(byte pos) {
  if (pos < 85) return strip.Color(pos*3, 255 - pos*3, 0);
  if (pos < 170) { pos -= 85; return strip.Color(255 - pos*3, 0, pos*3); }
  pos -= 170; return strip.Color(0, pos*3, 255 - pos*3);
}

// ---------- EFEKTY (neblokující) ----------
static inline uint16_t computeIntervalMs(uint16_t baseMs) {
  // speedPercent: 1..100 -> škáluje interval zhruba 2.0x (pomalé) až 0.25x (rychlé)
  float sp = (float)constrain(speedPercent, 1, 100) / 100.0f; // 0.01..1.00
  float scale = 2.0f - 1.75f * sp; // 2.0 .. 0.25
  uint16_t iv = (uint16_t)(baseMs * scale);
  if (iv < 5) iv = 5;
  return iv;
}

static inline uint32_t scaleColor(uint32_t c, uint8_t scale) {
  // scale: 0..255
  uint8_t r = (uint8_t)((((c >> 16) & 0xFF) * scale) >> 8);
  uint8_t g = (uint8_t)((((c >>  8) & 0xFF) * scale) >> 8);
  uint8_t b = (uint8_t)((((c      ) & 0xFF) * scale) >> 8);
  return strip.Color(r, g, b);
}

static inline void dimAll(uint8_t by) {
  // by ~ 0..255; 0 = no dim, 255 = clear
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    uint32_t c = strip.getPixelColor(i);
    strip.setPixelColor(i, scaleColor(c, 255 - by));
  }
}
void stepRainbow() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(25)) return; last = tNow;
  for (uint16_t i=0;i<strip.numPixels();i++)
    strip.setPixelColor(i, wheel((i*256/strip.numPixels() + rainbow.j) & 255));
  strip.show();
  rainbow.j = (rainbow.j + 1) & 0xFF;
}
void stepTheater() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(70)) return; last = tNow;
  strip.clear();
  uint32_t c = strip.Color(255, 60, 0);
  for (uint16_t i = theater.offset; i < strip.numPixels(); i += 3) strip.setPixelColor(i, c);
  strip.show();
  theater.offset = (theater.offset + 1) % 3;
}
void stepBounce() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(45)) return; last = tNow;
  strip.clear();
  uint32_t cMain = strip.Color(0, 180, 255), cDim = strip.Color(8,8,16);
  strip.setPixelColor(bounce.pos, cMain);
  if (bounce.pos>0) strip.setPixelColor(bounce.pos-1, cDim);
  if (bounce.pos<strip.numPixels()-1) strip.setPixelColor(bounce.pos+1, cDim);
  strip.show();
  bounce.pos += bounce.dir;
  if (bounce.pos<=0 || bounce.pos>=strip.numPixels()-1) bounce.dir = -bounce.dir;
}
void stepSparkle() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(30)) return; last = tNow;
  for (uint16_t i=0;i<strip.numPixels();i++) strip.setPixelColor(i, strip.Color(0,3,10));
  for (uint8_t s=0;s<2;s++) strip.setPixelColor(random(strip.numPixels()), strip.Color(255,255,255));
  strip.show();
}

// 4) Color Wipe (sekvenční vybarvování, mění barvy)
void stepWipe() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(20)) return; last = tNow;
  if (wipe.clearing) {
    strip.clear();
    wipe.idx = 0;
    wipe.clearing = false;
    // zvol náhodnou barvu mimo tmavé tóny
    uint8_t h = random(256);
    wipe.color = wheel(h);
  }
  if (wipe.idx < strip.numPixels()) {
    strip.setPixelColor(wipe.idx++, wipe.color);
    strip.show();
  } else {
    wipe.clearing = true;
  }
}

// 5) Twinkle (náhodné jiskření s pomalým zhasínáním)
void stepTwinkle() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(40)) return; last = tNow;
  dimAll(25);
  for (uint8_t k=0; k<2; k++) {
    uint16_t i = random(strip.numPixels());
    strip.setPixelColor(i, strip.Color(255, 255, 180));
  }
  strip.show();
}

// 6) Comet (kometa s ohonem)
void stepComet() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(25)) return; last = tNow;
  dimAll(20);
  strip.setPixelColor(comet.pos, strip.Color(100, 200, 255));
  strip.show();
  comet.pos += comet.dir;
  if (comet.pos<=0 || comet.pos>=strip.numPixels()-1) comet.dir = -comet.dir;
}

// 7) Fire (teplé mihotání)
void stepFire() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(35)) return; last = tNow;
  for (uint16_t i=0; i<strip.numPixels(); i++) {
    uint8_t heat = random(140, 255);
    uint8_t r = heat;
    uint8_t g = (uint8_t)(heat * 0.36f);
    uint8_t b = (uint8_t)(heat * 0.05f);
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}

// 8) Breathe (plynulé dýchání s fialovo-modrým gradientem)
void stepBreathe() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(20)) return; last = tNow;
  breathe.t = (breathe.t + 3) & 1023; // fáze 0..1023
  float x = (float)breathe.t / 1023.0f; // 0..1
  float s = 0.5f * (1.0f - cosf(2.0f * 3.1415926f * x)); // 0..1
  uint8_t a = (uint8_t)(s * 255);
  for (uint16_t i=0; i<strip.numPixels(); i++) {
    float t = (float)i / (strip.numPixels()-1);
    uint8_t r = (uint8_t)((uint16_t)(180*(1.0f-t) + 20*t) * a / 255);
    uint8_t g = (uint8_t)((uint16_t)( 20*(1.0f-t) + 40*t) * a / 255);
    uint8_t b = (uint8_t)((uint16_t)(220*(1.0f-t) + 255*t) * a / 255);
    strip.setPixelColor(i, strip.Color(r,g,b));
  }
  strip.show();
}

// 9) Larson Scanner (Cylon)
void stepScanner() {
  static unsigned long last=0; if (tNow - last < computeIntervalMs(30)) return; last = tNow;
  dimAll(40);
  strip.setPixelColor(scanner.pos, strip.Color(255, 30, 30));
  if (scanner.pos>0) strip.setPixelColor(scanner.pos-1, strip.Color(120, 10, 10));
  if (scanner.pos<strip.numPixels()-1) strip.setPixelColor(scanner.pos+1, strip.Color(120, 10, 10));
  strip.show();
  scanner.pos += scanner.dir;
  if (scanner.pos<=0 || scanner.pos>=strip.numPixels()-1) scanner.dir = -scanner.dir;
}

// ---------- PŘÍKAZY ZE STRÁNKY ----------
String rx;

void applyMode(uint8_t m) {
  currentMode = m % 10;
  rainbow = RainbowState{};
  theater = TheaterState{};
  bounce  = BounceState{};
  sparkle = SparkleState{};
  wipe    = WipeState{};
  twinkle = TwinkleState{};
  comet   = CometState{};
  fireFx  = FireState{};
  breathe = BreatheState{};
  scanner = ScannerState{};
}
void applyBrightness(uint8_t b) {
  brightness = b;
  strip.setBrightness(brightness);
  strip.show();
}
void applySpeed(uint8_t s) {
  if (s < 1) s = 1; if (s > 100) s = 100;
  speedPercent = s;
}

void handleLine(String line) {
  line.trim(); line.toUpperCase();
  if (line.startsWith("MODE")) {
    int sp = line.indexOf(' ');
    if (sp>0) { int m = line.substring(sp+1).toInt(); applyMode((uint8_t)m); }
    Serial.print("OK MODE "); Serial.println(currentMode);
    return;
  }
  if (line.startsWith("BRI")) {
    int sp = line.indexOf(' ');
    if (sp>0) {
      int v = line.substring(sp+1).toInt();
      if(v<0)v=0; if(v>255)v=255;
      applyBrightness((uint8_t)v);
    }
    Serial.print("OK BRI "); Serial.println(brightness);
    return;
  }
  if (line.startsWith("SPEED")) {
    int sp = line.indexOf(' ');
    if (sp>0) {
      int v = line.substring(sp+1).toInt();
      applySpeed((uint8_t)v);
    }
    Serial.print("OK SPEED "); Serial.println(speedPercent);
    return;
  }
}

// ---------- SETUP/LOOP ----------
void setup() {
  strip.begin();
  strip.setBrightness(brightness);
  strip.show();

  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);

  btn.attach(PIN_SW);
  btn.interval(8);

  lastAB = readAB();
  attachInterrupt(digitalPinToInterrupt(PIN_CLK), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_DT),  updateEncoder, CHANGE);

  Serial.begin(115200);
  // Lepší seed na ESP32:
  randomSeed((uint32_t)esp_random());
  applyMode(0);
  Serial.println("READY");
}

void loop() {
  tNow = millis();

  // Efekt podle módu
  switch (currentMode) {
    case 0: stepRainbow(); break;
    case 1: stepTheater(); break;
    case 2: stepBounce();  break;
    case 3: stepSparkle(); break;
    case 4: stepWipe();    break;
    case 5: stepTwinkle(); break;
    case 6: stepComet();   break;
    case 7: stepFire();    break;
    case 8: stepBreathe(); break;
    case 9: stepScanner(); break;
  }

  // Události z enkodéru → pošli stránce
  noInterrupts();
  int8_t d = encDelta;
  encDelta = 0;
  interrupts();

  if (d != 0) {
    if (ENC_INVERT) d = -d;
    encAccum += d;

    while (encAccum >= ENC_SCALE) {
      Serial.println("EV DOWN");
      encAccum -= ENC_SCALE;
    }
    while (encAccum <= -ENC_SCALE) {
      Serial.println("EV UP");
      encAccum += ENC_SCALE;
    }
  }

  btn.update();
  if (btn.fell()) Serial.println("EV PRESS");

  // Čti příkazy ze stránky
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch=='\n' || ch=='\r') {
      if (rx.length()) { handleLine(rx); rx=""; }
    } else {/******************************************************
ZAPOJENÍ (ESP32 + NeoPixel čtverec 8×8 = 64 LED)
-------------------------------------------------------
NeoPixel 8×8 (WS2812/WS2812B – 5 V logika i napájení):
- DIN (data)  -> ESP32 GPIO 5 (D5) přes odpor ~330 Ω
- 5V          -> 5V zdroj (USB nebo externí 5V; společná GND!)
- GND         -> GND ESP32
(Chraň LED zdroj: dej elektrolytický kondenzátor 470–1000 µF mezi 5V a GND u LED – správná polarita!)

POZNÁMKY PRO DĚTI:
- „Společná zem“ znamená, že GND zdroje i GND ESP32 musí být propojené.
- Nepřipojuj LED přímo na 3,3 V – potřebují 5 V (pro WS2812).
- Když to nesvítí, zkontroluj šipku „DIN →“ (směr dat do panelu).

OVLÁDÁNÍ:
- Otevři Serial Monitor (115200 baud).
- Napiš zprávu (např. AHOJ 123) a odešli (ENTER).
- Každé písmeno se zobrazí 750 ms. Mezitím běží „duhová show“.
- Písmeno funguje jako „maska“: LED, které nepatří do tvaru písmene, se vypnou.
  Ponechané LED svítí duhově (show běží pod písmenem).

*******************************************************
Úkoly, pro pochopení kódu (zkus odpovědět):
1) Proč máme odpor ~330 Ω v datové lince a kondenzátor mezi 5V a GND?
2) Co znamená „společná zem“ a proč je důležitá?
3) Jak kód překládá písmeno (např. 'A') na rozsvícené body 8×8?
4) Co dělá funkce mapXY(x, y)? Proč řeší „hadí“ (serpentinové) zapojení řádků?
5) Kdy se přepíná na další písmeno a jak se počítá čas (millis)?
6) Jak bys změnil rychlost duhy? Které proměnné upravit?
7) Jak přidat nový znak do fontu 5×7?
8) Jak bys udělal, aby „nepísmenné“ LED nebyly úplně zhasnuté, ale jen slabě svítily?
******************************************************/

#include <Adafruit_NeoPixel.h>

// ======= UPRAV DLE SVÉHO HARDWARE =======
#define LED_PIN        5          // GPIO pro data do NeoPixel panelu
#define WIDTH          8
#define HEIGHT         8
#define NUMPIXELS      (WIDTH*HEIGHT)
#define SERPENTINE     1          // 1 = sudý řádek L→R, lichý R→L (běžné u panelů)

// Rychlost duhy (čím větší, tím rychlejší posun)
#define RAINBOW_SPEED  2          // „krok“ posunu duhy za frame

// Čas zobrazení jednoho znaku (ms)
#define LETTER_DURATION 750

// Globální LED objekt
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ======= POMOCNÉ – MAPOVÁNÍ XY → INDEX =======
uint16_t mapXY(uint8_t x, uint8_t y) {
  if (SERPENTINE) {
    // Řádky 0,2,4,... zleva doprava; 1,3,5,... zprava doleva
    if (y % 2 == 0) {
      return y * WIDTH + x;
    } else {
      return y * WIDTH + (WIDTH - 1 - x);
    }
  } else {
    // Jednoduché řádkové mapování zleva doprava
    return y * WIDTH + x;
  }
}

// ======= DUHOVÉ BARVY (klasické „Wheel“) =======
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

// ======= 5×7 FONT (šířka 5, výška 7), každý znak = 5 sloupců (bity DOLŮ) =======
// Bit 0 = horní řádek, bit 6 = dolní řádek (0..6). Jeden byte = 1 sloupec (7 bitů využito).
// Podporované znaky: ' ' A-Z 0-9 . , ! ? - :
// (Stačí přidat další – pohlídej 5 sloupců na znak)
struct Glyph { char c; uint8_t col[5]; };

const Glyph FONT[] PROGMEM = {
  {' ', {0x00,0x00,0x00,0x00,0x00}},
  {'A', {0x1E,0x05,0x05,0x1E,0x00}}, // 0b0 11110, 0b0 00101 ...
  {'B', {0x1F,0x15,0x15,0x0A,0x00}},
  {'C', {0x0E,0x11,0x11,0x0A,0x00}},
  {'D', {0x1F,0x11,0x11,0x0E,0x00}},
  {'E', {0x1F,0x15,0x15,0x11,0x00}},
  {'F', {0x1F,0x05,0x05,0x01,0x00}},
  {'G', {0x0E,0x11,0x15,0x1D,0x00}},
  {'H', {0x1F,0x04,0x04,0x1F,0x00}},
  {'I', {0x11,0x1F,0x11,0x00,0x00}},
  {'J', {0x08,0x10,0x10,0x0F,0x00}},
  {'K', {0x1F,0x04,0x0A,0x11,0x00}},
  {'L', {0x1F,0x10,0x10,0x10,0x00}},
  {'M', {0x1F,0x02,0x04,0x02,0x1F}}, // 5 sloupců, poslední přesah -> upravíme na 5:
       // Lepší varianta pro 5 sloupců:
  {'M', {0x1F,0x02,0x04,0x02,0x1F}},
  {'N', {0x1F,0x02,0x04,0x1F,0x00}},
  {'O', {0x0E,0x11,0x11,0x0E,0x00}},
  {'P', {0x1F,0x05,0x05,0x02,0x00}},
  {'Q', {0x0E,0x11,0x19,0x1E,0x00}},
  {'R', {0x1F,0x05,0x0D,0x12,0x00}},
  {'S', {0x12,0x15,0x15,0x09,0x00}},
  {'T', {0x01,0x1F,0x01,0x01,0x00}},
  {'U', {0x0F,0x10,0x10,0x0F,0x00}},
  {'V', {0x07,0x08,0x10,0x08,0x07}},
  {'W', {0x1F,0x08,0x04,0x08,0x1F}},
  {'X', {0x1B,0x04,0x04,0x1B,0x00}},
  {'Y', {0x03,0x04,0x18,0x04,0x03}},
  {'Z', {0x19,0x15,0x13,0x11,0x00}},
  {'0', {0x0E,0x11,0x11,0x0E,0x00}},
  {'1', {0x12,0x1F,0x10,0x00,0x00}},
  {'2', {0x19,0x15,0x15,0x12,0x00}},
  {'3', {0x11,0x15,0x15,0x0A,0x00}},
  {'4', {0x07,0x04,0x04,0x1F,0x00}},
  {'5', {0x17,0x15,0x15,0x09,0x00}},
  {'6', {0x0E,0x15,0x15,0x08,0x00}},
  {'7', {0x01,0x01,0x1D,0x03,0x00}},
  {'8', {0x0A,0x15,0x15,0x0A,0x00}},
  {'9', {0x02,0x15,0x15,0x0E,0x00}},
  {'.', {0x00,0x10,0x00,0x00,0x00}},
  {',', {0x00,0x10,0x08,0x00,0x00}},
  {'!', {0x00,0x1D,0x00,0x00,0x00}},
  {'?', {0x01,0x15,0x05,0x02,0x00}},
  {'-', {0x04,0x04,0x04,0x04,0x00}},
  {':', {0x00,0x0A,0x00,0x00,0x00}},
};

const uint8_t FONT_LEN = sizeof(FONT) / sizeof(FONT[0]);

// Najdi glyph pro znak c (A..Z převede na velké)
bool getGlyph(char c, uint8_t outCols[5]) {
  if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
  for (uint8_t i = 0; i < FONT_LEN; i++) {
    Glyph g;
    memcpy_P(&g, &FONT[i], sizeof(Glyph));
    if (g.c == c) {
      for (uint8_t k = 0; k < 5; k++) {
        outCols[k] = pgm_read_byte(&(((Glyph*)(&FONT[i]))->col[k]));
      }
      return true;
    }
  }
  // neznámý znak -> mezera
  for (uint8_t k = 0; k < 5; k++) outCols[k] = 0x00;
  return false;
}

// ======= TEXT A ČASOVAČE =======
String message = "AHOJ 123";   // výchozí zpráva
uint16_t msgIndex = 0;
uint32_t letterStartMs = 0;

uint16_t rainbowOffset = 0;     // posun duhy

// ======= VYKRESLENÍ RÁMCE =======
void renderFrame(char currentChar) {
  // 1) zjisti 5 sloupců 5×7 pro currentChar
  uint8_t cols[5];
  getGlyph(currentChar, cols);

  // 2) připrav masku 8×8: true = patří do písmene
  bool mask[HEIGHT][WIDTH];
  for (uint8_t y = 0; y < HEIGHT; y++)
    for (uint8_t x = 0; x < WIDTH; x++)
      mask[y][x] = false;

  // Písmo 5×7 centrované do 8×8:
  const int sx = 1; // start X (1 pixel odsazení zleva)
  const int sy = 0; // start Y (nahoře)
  for (uint8_t cx = 0; cx < 5; cx++) {
    uint8_t colBits = cols[cx]; // 7 bitů použito (b0 nahoře)
    for (uint8_t cy = 0; cy < 7; cy++) {
      bool on = (colBits >> cy) & 0x01;
      int x = sx + cx;
      int y = sy + cy;
      if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) {
        mask[y][x] = on;
      }
    }
  }

  // 3) vykresli duhu a filtruj maskou
  for (uint8_t y = 0; y < HEIGHT; y++) {
    for (uint8_t x = 0; x < WIDTH; x++) {
      uint16_t idx = mapXY(x, y);
      // Podkladová duha – využijeme index + řádek + časový offset
      uint8_t hue = (uint8_t)((idx * 256 / NUMPIXELS + rainbowOffset) & 0xFF);
      uint32_t rgb = wheel(hue);

      if (mask[y][x]) {
        // LED patří do tvaru znaku → necháme ji svítit duhově
        strip.setPixelColor(idx, rgb);
      } else {
        // LED mimo znak → zhasnout (filtr)
        strip.setPixelColor(idx, 0);
      }
    }
  }
  strip.show();
}

// ======= ČTENÍ ŘÁDKU ZE SERIAL =======
bool readLineFromSerial(String &out) {
  static String buf;
  while (Serial.available()) {
    char ch = (char)Serial.read();
    if (ch == '\r') continue;  // ignoruj CR
    if (ch == '\n') {
      out = buf;
      buf = "";
      return true;
    } else {
      buf += ch;
    }
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  strip.begin();
  strip.setBrightness(80); // můžeš upravit
  strip.show();

  Serial.println(F("ESP32 NeoPixel 8x8 – textová maska na duhové show"));
  Serial.println(F("Napiš zprávu a stiskni ENTER. Podporované: A-Z, 0-9, mezera a několik znaků .,!?:-"));
  Serial.println(F("Každé písmeno se ukazuje 750 ms."));

  letterStartMs = millis();
}

void loop() {
  // 1) přijmi novou zprávu (neblokující)
  String line;
  if (readLineFromSerial(line)) {
    line.trim();
    if (line.length() == 0) {
      Serial.println(F("(Zpráva prázdná) – ponechávám předchozí."));
    } else {
      message = line;
      msgIndex = 0;
      letterStartMs = millis();
      Serial.print(F("Nová zpráva: \""));
      Serial.print(message);
      Serial.println(F("\""));
    }
  }

  // 2) vyber aktuální znak (když zpráva prázdná, zobraz mezery)
  char c = ' ';
  if (message.length() > 0) {
    c = message.charAt(msgIndex % message.length());
  }

  // 3) vykresli rámec (duha + maska znaku)
  renderFrame(c);

  // 4) posuň duhu pro další frame (plynulá animace)
  rainbowOffset = (rainbowOffset + RAINBOW_SPEED) & 0xFF;

  // 5) po 750 ms přepni na další písmeno
  if (millis() - letterStartMs >= LETTER_DURATION) {
    msgIndex = (msgIndex + 1) % max(1, (int)message.length());
    letterStartMs = millis();
  }

  // 6) udržuj příjemný FPS (cca 60–120Hz není třeba; klidně 100 FPS → delay(5))
  delay(5);
}

      rx += ch;
      if (rx.length()>64) rx="";
    }
  }
}
