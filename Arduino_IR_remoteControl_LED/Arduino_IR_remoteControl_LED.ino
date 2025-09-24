/**************************************************************
ZAPOJENÍ (pro děti, stručně):
1) IR přijímač:
   - OUT (datový pin) → Arduino D2
   - VCC → 5V
   - GND → GND

2) LED pásek WS2815 (60 LED):
   - DIN (šipka směrem DOVNITŘ) → Arduino D6
   - V+ → 12 V z externího zdroje (NE do Arduina!)
   - GND → GND zdroje
   - Propoj GND zdroje s GND Arduina (společná zem).
   - Doporučeno: rezistor 330–470 Ω do série s DIN, kondenzátor 1000 µF mezi 12 V a GND na začátku pásku.

3) Napájení:
   - WS2815 jsou na 12 V. Arduino běží z USB.
   - Nikdy nepřipojuj 12 V do Arduina. Jen propojit země.

OVLÁDÁNÍ DÁLKOU:
- Tlačítka 1–9: volí 9 dynamických světelných show.
- # (HASH): zhasne (OFF).
- UP/DOWN: jas (0–255).
- LEFT/RIGHT: rychlost animace.

Úkoly, pro pochopení kódu:
1) Kde se v kódu nastavuje počet LED a pin pro pásek?
2) Jak kód řeší opakování IR signálu (NEC repeat 0x0)?
3) Která proměnná ovlivňuje rychlost a jak se mapuje na čas snímku?
4) Najdi v kódu místo, kde se přepínají efekty na číslech 1–9.
5) Jak funguje „ne-blokující“ animace s millis() místo delay()?
**************************************************************/

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp>          // IRremote v3+
#include <MyIRcodes.h>
using namespace MyIR;   // now ONE, TWO, LEFTBTN, ... are visible without prefix

// ====== Uživatelské nastavení ======
#define LED_PIN       6
#define IR_PIN        2
#define NUM_LEDS      60          // MyLEDStrip = 60 WS2815
#define LED_TYPE_FLAGS (NEO_GRB + NEO_KHZ800)

// Rychlost: index 0..10 se mapuje na interval snímku (ms)
const uint8_t SPEED_MIN_INDEX = 0;
const uint8_t SPEED_MAX_INDEX = 10;
const uint16_t SPEED_TABLE_MS[] = { 15, 20, 25, 33, 40, 50, 66, 80, 100, 120, 150 };

// Jas: 0..255 (Adafruit_NeoPixel vnitřně škáluje)
const uint8_t BRIGHTNESS_STEP = 16; // změna po krocích

// ====== Globální stav ======
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, LED_TYPE_FLAGS);

volatile uint8_t  brightness   = 128;  // startovní jas
volatile uint8_t  speedIndex   = 6;    // startovní rychlost (střed)
volatile uint8_t  currentEffect = 1;   // 1..9; 0 = OFF
volatile bool     isOff = false;

unsigned long lastFrameAt = 0;
unsigned long lastButton  = 0;      // poslední platné tlačítko (pro repeat)
uint32_t frameCounter = 0;

// Pomocné buffery pro efekty
uint8_t  theaterPhase = 0;
int      wipePos = 0;      int wipeDir = +1;
int      cometPos = 0;     int cometDir = +1;
float    breathePhase = 0; // 0..2π
int      meteorHeadL = 0, meteorHeadR = NUM_LEDS-1;

// ====== Pomocné funkce ======
const char* buttonName(unsigned long code) {
  if (code == ONE)        return "ONE";
  if (code == TWO)        return "TWO";
  if (code == THREE)      return "THREE";
  if (code == FOUR)       return "FOUR";
  if (code == FIVE)       return "FIVE";
  if (code == SIX)        return "SIX";
  if (code == SEVEN)      return "SEVEN";
  if (code == EIGHT)      return "EIGHT";
  if (code == NINE)       return "NINE";
  if (code == ZERO)       return "ZERO";
  if (code == STARBTN)    return "STAR";
  // Směrovky + hash/OK dle vaší hlavičky:
  if (code == LEFTBTN)    return "LEFT";
  if (code == RIGHTBTN)   return "RIGHT";
  if (code == UPBTN)      return "UP";
  if (code == DOWNBTN)    return "DOWN";
  if (code == OKBTN)      return "OK";
  if (code == HASHBTN)    return "HASH";
  return "UNKNOWN";
}

void logIR(unsigned long code) {
  Serial.print("IR raw=0x"); Serial.print(code, HEX);
  Serial.print("  btn="); Serial.println(buttonName(code));
}

// Bezpečné nastavení jasu
void applyBrightness() {
  strip.setBrightness(brightness);
}

// Mapování rychlosti na interval
uint16_t currentFrameInterval() {
  return SPEED_TABLE_MS[constrain(speedIndex, SPEED_MIN_INDEX, SPEED_MAX_INDEX)];
}

// Jemné ztmavení celé lišty (pro twinkle/confetti apod.)
void fadeAll(uint8_t amount) {
  for (int i=0; i<NUM_LEDS; i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >>  8) & 0xFF;
    uint8_t b = (c      ) & 0xFF;
    r = (r > amount) ? r - amount : 0;
    g = (g > amount) ? g - amount : 0;
    b = (b > amount) ? b - amount : 0;
    strip.setPixelColor(i, r, g, b);
  }
}

// Rychlý pomocník HSV (využijeme vestavěné ColorHSV)
uint32_t hsv(uint16_t h, uint8_t s=255, uint8_t v=255) {
  return strip.gamma32(strip.ColorHSV(h, s, v));
}

// ====== Efekty ======
// 1) Rainbow Wheel
void fxRainbowWheel() {
  for (int i=0; i<NUM_LEDS; i++) {
    uint16_t hue = (i * 65535UL / NUM_LEDS + frameCounter * 256) & 0xFFFF;
    strip.setPixelColor(i, hsv(hue));
  }
  strip.show();
}

// 2) Comet (světlá hlava, doznívající ocas)
void fxComet() {
  fadeAll(20);
  strip.setPixelColor(cometPos, hsv((frameCounter*1024) & 0xFFFF, 255, 255));
  strip.show();

  cometPos += cometDir;
  if (cometPos <= 0 || cometPos >= NUM_LEDS-1) cometDir = -cometDir;
}

// 3) Theater Chase (běžící body)
void fxTheaterChase() {
  strip.clear();
  for (int i=theaterPhase; i<NUM_LEDS; i+=3) {
    strip.setPixelColor(i, hsv((i*3000 + frameCounter*2000) & 0xFFFF));
  }
  strip.show();
  theaterPhase = (theaterPhase + 1) % 3;
}

// 4) Breathe (nádech-výdech celé lišty)
void fxBreathe() {
  // plynulý jas podle sinusoidy
  float speed = 0.08f; // základní rychlost dýchání (nezávislá na speedIndex, už tak je to příjemné)
  breathePhase += speed;
  if (breathePhase > TWO_PI) breathePhase -= TWO_PI;

  uint8_t v = (uint8_t)( (sin(breathePhase) * 0.5f + 0.5f) * 255 );
  uint16_t baseHue = (frameCounter*256) & 0xFFFF;

  for (int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, hsv(baseHue, 255, v));
  }
  strip.show();
}

// 5) Color Wipe Ping-Pong
void fxWipePingPong() {
  uint16_t hue = (frameCounter*1024) & 0xFFFF;
  strip.setPixelColor(wipePos, hsv(hue));
  strip.show();

  wipePos += wipeDir;
  if (wipePos >= NUM_LEDS) { wipePos = NUM_LEDS-1; wipeDir = -1; }
  if (wipePos < 0)         { wipePos = 0;         wipeDir = +1; }
}

// 6) Twinkles (náhodné jiskry)
void fxTwinkles() {
  fadeAll(10);
  // pár náhodných jisker
  for (int k=0; k<2; k++) {
    int i = random(NUM_LEDS);
    uint16_t hue = random(65536);
    strip.setPixelColor(i, hsv(hue));
  }
  strip.show();
}

// 7) Palette Drift (plynulá změna odstínů po liště)
void fxPaletteDrift() {
  for (int i=0; i<NUM_LEDS; i++) {
    uint16_t hue = (i*1500 + frameCounter*400) & 0xFFFF;
    strip.setPixelColor(i, hsv(hue, 255, 255));
  }
  strip.show();
}

// 8) Meteor Rain (zleva i zprava)
void fxMeteorDual() {
  fadeAll(25);
  strip.setPixelColor(meteorHeadL, hsv((frameCounter*1400) & 0xFFFF));
  strip.setPixelColor(meteorHeadR, hsv((65535 - frameCounter*1400) & 0xFFFF));
  strip.show();

  meteorHeadL = (meteorHeadL + 1) % NUM_LEDS;
  meteorHeadR = (meteorHeadR - 1 + NUM_LEDS) % NUM_LEDS;
}

// 9) Confetti
void fxConfetti() {
  fadeAll(8);
  int i = random(NUM_LEDS);
  uint16_t hue = (frameCounter*1800 + i*3000) & 0xFFFF;
  strip.setPixelColor(i, hsv(hue));
  strip.show();
}

// Reset stavů efektů při přepnutí
void resetEffects() {
  theaterPhase = 0;
  wipePos = 0; wipeDir = +1;
  cometPos = 0; cometDir = +1;
  breathePhase = 0;
  meteorHeadL = 0; meteorHeadR = NUM_LEDS-1;
  frameCounter = 0;
}

// ====== IR handling ======
void handleButton(unsigned long signal) {
  // Pokud přišel NEC repeat (0x0), použij poslední tlačítko
  if (signal == 0x0 && lastButton != 0) {
    signal = lastButton;
  } else if (signal != 0x0) {
    lastButton = signal;
  }

  // Debug log
  logIR(signal);

  // Přepínání efektů 1..9
  if (signal == ONE)   { currentEffect = 1; isOff = false; resetEffects(); return; }
  if (signal == TWO)   { currentEffect = 2; isOff = false; resetEffects(); return; }
  if (signal == THREE) { currentEffect = 3; isOff = false; resetEffects(); return; }
  if (signal == FOUR)  { currentEffect = 4; isOff = false; resetEffects(); return; }
  if (signal == FIVE)  { currentEffect = 5; isOff = false; resetEffects(); return; }
  if (signal == SIX)   { currentEffect = 6; isOff = false; resetEffects(); return; }
  if (signal == SEVEN) { currentEffect = 7; isOff = false; resetEffects(); return; }
  if (signal == EIGHT) { currentEffect = 8; isOff = false; resetEffects(); return; }
  if (signal == NINE)  { currentEffect = 9; isOff = false; resetEffects(); return; }

  // # HASH → OFF
  if (signal == HASHBTN) {
    isOff = true;
    strip.clear(); strip.show();
    return;
  }

  // Jas UP/DOWN
  if (signal == UPBTN) {
    brightness = (brightness > 255 - BRIGHTNESS_STEP) ? 255 : brightness + BRIGHTNESS_STEP;
    applyBrightness();
    Serial.print("Brightness: "); Serial.println(brightness);
    return;
  }
  if (signal == DOWNBTN) {
    brightness = (brightness < BRIGHTNESS_STEP) ? 0 : brightness - BRIGHTNESS_STEP;
    applyBrightness();
    Serial.print("Brightness: "); Serial.println(brightness);
    return;
  }

  // Rychlost LEFT/RIGHT
  if (signal == LEFTBTN) {
    if (speedIndex > SPEED_MIN_INDEX) speedIndex--;
    Serial.print("SpeedIndex: "); Serial.println(speedIndex);
    return;
  }
  if (signal == RIGHTBTN) {
    if (speedIndex < SPEED_MAX_INDEX) speedIndex++;
    Serial.print("SpeedIndex: "); Serial.println(speedIndex);
    return;
  }

  // OK – volitelně pauza/resume (nepožadováno) – zde neděláme nic
}

// ====== Arduino standard ======
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println(F("IR + WS2815 controller starting..."));

  // LED strip
  strip.begin();
  strip.clear();
  strip.setBrightness(brightness);
  strip.show();

  // IR receiver
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("IR prijimac pripraven (IRremote v3+). Zmackni tlacitko na ovladaci."));
}

void loop() {
  // Příjem IR
  if (IrReceiver.decode()) {
    unsigned long signal = IrReceiver.decodedIRData.decodedRawData;
    handleButton(signal);
    IrReceiver.resume(); // připrav dekodér na další kód
  }

  // Animace – neblokující
  if (isOff || currentEffect == 0) {
    // OFF: nic neděláme
    return;
  }

  unsigned long now = millis();
  if (now - lastFrameAt >= currentFrameInterval()) {
    lastFrameAt = now;
    frameCounter++;

    switch (currentEffect) {
      case 1: fxRainbowWheel();  break;
      case 2: fxComet();         break;
      case 3: fxTheaterChase();  break;
      case 4: fxBreathe();       break;
      case 5: fxWipePingPong();  break;
      case 6: fxTwinkles();      break;
      case 7: fxPaletteDrift();  break;
      case 8: fxMeteorDual();    break;
      case 9: fxConfetti();      break;
      default: /* bezpečná výchozí */ fxRainbowWheel(); break;
    }
  }
}
