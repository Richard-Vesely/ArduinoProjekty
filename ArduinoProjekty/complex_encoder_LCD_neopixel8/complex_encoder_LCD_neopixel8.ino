  /*
  ==================== ZAPOJENÍ ====================

  Arduino Leonardo  ->  NeoPixel 8 (WS2812/WS2812B)
  -------------------------------------------------
  5V                ->  +5V (VIN) LED
  GND               ->  GND LED  (společná zem s Arduinem i PC)
  D6                ->  DIN LED  (doporučeno 470 Ω v sérii)
  Kondenzátor 1000 µF mezi +5V a GND u LED (správná polarita)

  Arduino Leonardo  ->  LCD2004 I2C (PCF8574 převodník)
  -----------------------------------------------------
  GND               ->  GND
  5V                ->  VCC
  D2 (SDA)          ->  SDA
  D3 (SCL)          ->  SCL
  Typická I2C adresa: 0x27 (případně 0x3F)

  Arduino Leonardo  ->  Rotační enkodér (mechanický)
  --------------------------------------------------
  GND               ->  GND
  D7                ->  CLK / A (INPUT_PULLUP)
  D8                ->  DT  / B (INPUT_PULLUP)
  D4                ->  SW  (tlačítko; INPUT_PULLUP)

  Poznámky:
  - Pro LCD na Leonardu se běžně používají vyhrazené piny SDA/SCL – bývají i na headeru, ale funkčně to jsou D2/D3.
  - U enkodéru používám D7/D8 (ať se neplete s I2C na D2/D3). Jsou to běžné piny – dekódujeme softwarově.
  - Pokud má enkodér modul s pinem „+“, připoj jej na 5V (napájí jeho pull-up/LED).
  - Všechny země MUSÍ být společné.

  ==================================================
  UI chování:
  - Otáčením měníš výběr v matici 1..9 (arrow-like, s wrapem i bez – viz kód).
  - Stisk (SW) v menu -> přepne do detailu (zobrazí slovní název, spustí efekt).
  - Stisk v detailu -> zpět do menu.

  Enkodér:
  - scale = 2  (2 kvadraturní kroky = 1 „logický“ krok)

  ==================================================
  */

  #include <Adafruit_NeoPixel.h>
  #include <LiquidCrystal_I2C.h>
  #include <Bounce2.h>

  // -------------------- HW KONFIG --------------------
  #define LED_PIN    6
  #define LED_COUNT  8
  #define DEFAULT_BRIGHTNESS 40

  // LCD (změň adresu pokud máš jinou)
  #define LCD_ADDR   0x27
  #define LCD_COLS   20
  #define LCD_ROWS   4

  // Enkodér piny (A/B/Tlačítko)
  const uint8_t PIN_CLK = 7;  // A / CLK
  const uint8_t PIN_DT  = 8;  // B / DT
  const uint8_t PIN_SW  = 4;  // tlačítko (na GND)

  // Enkoder scale & směr
  const int8_t ENC_SCALE   = 2;    // požadováno: scale 2
  const bool   ENC_INVERT  = false;

  // -------------------- GLOBÁLY --------------------
  Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
  LiquidCrystal_I2C lcd(LCD_ADDR, LCD_COLS, LCD_ROWS);
  Bounce btn = Bounce();

  unsigned long tNow;

  // --- Serial command buffer ---
  char serialBuf[64];
  uint8_t serialLen = 0;

  // --- Encoder dekodér (soft) ---
  volatile int8_t encDelta = 0;    // syrové kroky z dekodéru
  volatile uint8_t lastAB  = 0;
  volatile int16_t encAccum = 0;   // akumulátor pro scale

  // 4-stavová tabulka (kvadraturní Gray)
  int8_t qdecTable[16] = {
    0, -1, +1,  0,
    +1, 0,  0, -1,
    -1, 0,  0, +1,
    0, +1, -1, 0
  };

  inline uint8_t readAB() {
    uint8_t a = !digitalRead(PIN_CLK); // INPUT_PULLUP -> invert
    uint8_t b = !digitalRead(PIN_DT);
    return (a << 1) | b;
  }

  void updateEncoderSoft() {
    uint8_t newAB = readAB();
    uint8_t idx = (lastAB << 2) | newAB;
    encDelta += qdecTable[idx];
    lastAB = newAB;
  }

  // --- UI stav ---
  enum UiState { UI_MENU, UI_DETAIL };
  UiState ui = UI_MENU;

  uint8_t selIndex = 0;     // 0..8 (mapuje na 1..9)
  uint8_t lastSel  = 255;   // kvůli rychlému překreslení

  // Názvy (česky)
  const char* NAMES[9] = {
    "jedna","dva","tri","ctyri","pet","sest","sedm","osm","devet"
  };

  // --- Efekty ---
  uint8_t currentMode = 1;     // 1..9
  uint8_t brightness  = DEFAULT_BRIGHTNESS;

  // Stavové struktury pro efekty
  struct RainbowState { uint16_t j=0; unsigned long last=0; } stRainbow;
  struct TheaterState { uint8_t offset=0; unsigned long last=0; } stTheater;
  struct BounceState  { int pos=0; int dir=1; unsigned long last=0; } stBounce;
  struct SparkleState { unsigned long last=0; } stSparkle;
  struct WipeState    { uint16_t i=0; unsigned long last=0; bool forward=true; uint32_t color; } stWipe;
  struct LarsonState  { int pos=0; int dir=1; unsigned long last=0; } stLarson;
  struct CometState   { float pos=0; float vel=0.35; unsigned long last=0; } stComet;
  struct BreathState  { float phase=0; unsigned long last=0; } stBreath;

  // -------------------- HELPERY --------------------
  uint32_t wheel(byte pos) {
    if (pos < 85) return strip.Color(pos*3, 255 - pos*3, 0);
    if (pos < 170) { pos -= 85; return strip.Color(255 - pos*3, 0, pos*3); }
    pos -= 170; return strip.Color(0, pos*3, 255 - pos*3);
  }

  void setAll(uint32_t c) {
    for (uint16_t i=0;i<strip.numPixels();i++) strip.setPixelColor(i,c);
  }

  void resetAllEffects() {
    stRainbow = RainbowState{};
    stTheater = TheaterState{};
    stBounce  = BounceState{};
    stSparkle = SparkleState{};
    stWipe    = WipeState{};
    stLarson  = LarsonState{};
    stComet   = CometState{};
    stBreath  = BreathState{};
  }

  // -------------------- LCD UI --------------------
  void centerPrint(uint8_t row, const char* text) {
    int len = strlen(text);
    int col = (LCD_COLS - len)/2;
    if (col < 0) col = 0;
    lcd.setCursor(col, row);
    lcd.print(text);
  }

  void setupDisplay() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    centerPrint(0, "NeoPixel Demo 1-9");
    centerPrint(1, "Otaceni = vyber");
    centerPrint(2, "Stisk = detail");
    centerPrint(3, " ");
    delay(800);
  }

  void renderMenu() {
    lcd.clear();
    // 3x3 grid (řádky 0..2; sloupec zhruba uprostřed tří zón)
    const uint8_t rows[3] = {0,1,2};
    const uint8_t cols[3] = {3, 10, 17}; // vizuálně hezké rozložení v 20 sloupcích

    uint8_t n = 1;
    for (uint8_t r=0;r<3;r++) {
      for (uint8_t c=0;c<3;c++) {
        lcd.setCursor(cols[c]-1, rows[r]); // -1 kvůli rámečku [n]
        if (selIndex == (n-1)) {
          lcd.print('['); lcd.print(n); lcd.print(']');
        } else {
          lcd.print(' '); lcd.print(n); lcd.print(' ');
        }
        n++;
      }
    }
    lcd.setCursor(0,3);
    lcd.print("Vyber: ");
    lcd.print(selIndex+1);
    lcd.print("  (stisk=detail)");
    lastSel = selIndex;
  }

  void renderSelection() {
    if (ui != UI_MENU) return;
    if (lastSel == selIndex) return;
    const uint8_t rows[3] = {0,1,2};
    const uint8_t cols[3] = {3, 10, 17};

    // starý přepiš bez rámečku
    uint8_t old = lastSel;
    uint8_t orow = rows[old/3];
    uint8_t ocol = cols[old%3];
    lcd.setCursor(ocol-1, orow);
    lcd.print(' '); lcd.print((int)old+1); lcd.print(' ');

    // nový s rámečkem
    uint8_t nr = rows[selIndex/3];
    uint8_t nc = cols[selIndex%3];
    lcd.setCursor(nc-1, nr);
    lcd.print('['); lcd.print((int)selIndex+1); lcd.print(']');

    lcd.setCursor(7,3);
    lcd.print("   "); // vymaž staré číslo
    lcd.setCursor(7,3);
    lcd.print(selIndex+1);

    lastSel = selIndex;
  }

  void renderDetail(uint8_t idx) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Cislo: ");
    lcd.print(idx+1);
    lcd.setCursor(0,1);
    lcd.print("Nazev: ");
    lcd.print(NAMES[idx]);
    lcd.setCursor(0,3);
    lcd.print("[Stisk] Zpet  Rezim:");
    lcd.print(idx+1);
  }

  void enterMenu()  { ui = UI_MENU;  renderMenu(); }
  void enterDetail(){ ui = UI_DETAIL; renderDetail(selIndex); }

  // -------------------- APLIKACE MÓDU & JASU --------------------
  void applyMode(uint8_t m) {
    if (m < 1) m = 1;
    if (m > 9) m = 9;
    currentMode = m;
    resetAllEffects();
    Serial.print(F("[MODE] Now: ")); Serial.println(currentMode);
    // status na LCD v detailu
    if (ui == UI_DETAIL) {
      lcd.setCursor(16,3);
      lcd.print("   ");
      lcd.setCursor(16,3);
      lcd.print((int)currentMode);
    }
  }

  void applyBrightness(uint8_t b) {
    brightness = b;
    strip.setBrightness(brightness);
    strip.show();
    Serial.print(F("[BRI] Now: ")); Serial.println(brightness);
  }

  // -------------------- EFEKTY (krokové) --------------------

  // 1) Rainbow roll
  void stepRainbow() {
    if (tNow - stRainbow.last < 25) return; stRainbow.last = tNow;
    for (uint16_t i=0;i<strip.numPixels();i++)
      strip.setPixelColor(i, wheel((i*256/strip.numPixels() + stRainbow.j) & 255));
    strip.show();
    stRainbow.j = (stRainbow.j + 1) & 0xFF;
  }

  // 2) Theater chase
  void stepTheater() {
    if (tNow - stTheater.last < 70) return; stTheater.last = tNow;
    strip.clear();
    uint32_t c = strip.Color(255, 60, 0);
    for (uint16_t i = stTheater.offset; i < strip.numPixels(); i += 3) strip.setPixelColor(i, c);
    strip.show();
    stTheater.offset = (stTheater.offset + 1) % 3;
  }

  // 3) Bounce
  void stepBounce() {
    if (tNow - stBounce.last < 45) return; stBounce.last = tNow;
    strip.clear();
    uint32_t cMain = strip.Color(0, 180, 255), cDim = strip.Color(8,8,16);
    strip.setPixelColor(stBounce.pos, cMain);
    if (stBounce.pos>0) strip.setPixelColor(stBounce.pos-1, cDim);
    if (stBounce.pos<strip.numPixels()-1) strip.setPixelColor(stBounce.pos+1, cDim);
    strip.show();
    stBounce.pos += stBounce.dir;
    if (stBounce.pos<=0 || stBounce.pos>=strip.numPixels()-1) stBounce.dir = -stBounce.dir;
  }

  // 4) Sparkle on dark
  void stepSparkle() {
    if (tNow - stSparkle.last < 30) return; stSparkle.last = tNow;
    for (uint16_t i=0;i<strip.numPixels();i++) strip.setPixelColor(i, strip.Color(0,3,10));
    for (uint8_t s=0;s<2;s++) strip.setPixelColor(random(strip.numPixels()), strip.Color(255,255,255));
    strip.show();
  }

  // 5) Color wipe forward
  void stepColorWipeFwd() {
    if (stWipe.color == 0) stWipe.color = strip.Color(255, 40, 0); // init barva
    if (tNow - stWipe.last < 35) return; stWipe.last = tNow;
    if (stWipe.i < strip.numPixels()) {
      strip.setPixelColor(stWipe.i, stWipe.color);
      strip.show();
      stWipe.i++;
    } else {
      stWipe.i = 0;
      // změna barvy pro další průchod
      stWipe.color = strip.Color(random(40,255), random(30,200), random(30,200));
      strip.clear(); strip.show();
    }
  }

  // 6) Color wipe reverse
  void stepColorWipeRev() {
    if (stWipe.color == 0) stWipe.color = strip.Color(0, 120, 255);
    if (tNow - stWipe.last < 35) return; stWipe.last = tNow;
    if (stWipe.i < strip.numPixels()) {
      uint16_t idx = strip.numPixels()-1 - stWipe.i;
      strip.setPixelColor(idx, stWipe.color);
      strip.show();
      stWipe.i++;
    } else {
      stWipe.i = 0;
      stWipe.color = strip.Color(random(30,200), random(40,255), random(30,200));
      strip.clear(); strip.show();
    }
  }

  // 7) Larson scanner (Cylon)
  void stepLarson() {
    if (tNow - stLarson.last < 40) return; stLarson.last = tNow;
    strip.clear();
    // hlavní bod
    strip.setPixelColor(stLarson.pos, strip.Color(255,0,0));
    // doznívání
    if (stLarson.pos>0) strip.setPixelColor(stLarson.pos-1, strip.Color(60,0,0));
    if (stLarson.pos<strip.numPixels()-1) strip.setPixelColor(stLarson.pos+1, strip.Color(60,0,0));
    strip.show();
    stLarson.pos += stLarson.dir;
    if (stLarson.pos<=0 || stLarson.pos>=strip.numPixels()-1) stLarson.dir = -stLarson.dir;
  }

  // 8) Comet (plynulý pohyb s chvostem)
  void stepComet() {
    if (tNow - stComet.last < 25) return; stComet.last = tNow;
    strip.clear();
    for (int i=0;i<strip.numPixels();i++) {
      float d = fabs(i - stComet.pos);
      int b = (int)(max(0.0f, 255.0f - d*120.0f));
      strip.setPixelColor(i, strip.Color(b, b/3, 255 - b));
    }
    strip.show();
    stComet.pos += stComet.vel;
    if (stComet.pos >= strip.numPixels()-1 || stComet.pos <= 0) stComet.vel = -stComet.vel;
  }

  // 9) Breathing (nádech/výdech)
  void stepBreathing() {
    if (tNow - stBreath.last < 20) return; stBreath.last = tNow;
    stBreath.phase += 0.06f; // rychlost
    float s = (sin(stBreath.phase) * 0.5f + 0.5f); // 0..1
    int b = (int)(s * 255);
    setAll(strip.Color(b, b, b));
    strip.show();
  }

  // Vyber step podle currentMode
  void stepEffect() {
    switch (currentMode) {
      case 1: stepRainbow();       break;
      case 2: stepTheater();       break;
      case 3: stepBounce();        break;
      case 4: stepSparkle();       break;
      case 5: stepColorWipeFwd();  break;
      case 6: stepColorWipeRev();  break;
      case 7: stepLarson();        break;
      case 8: stepComet();         break;
      case 9: stepBreathing();     break;
    }
  }

  // -------------------- SETUP / LOOP --------------------
  void setup() {
    // Serial
    Serial.begin(115200);
    while (!Serial) { ; }
    Serial.println(F("HELLO Leonardo UI/Encoder/LCD/NeoPixel"));
    Serial.println(F("Commands: MODE n (1..9), BRI n (0..255), UP, DOWN, PRESS, ?, HELLO"));

    // NeoPixel
    strip.begin();
    strip.setBrightness(DEFAULT_BRIGHTNESS);
    strip.show();

    // Enkodér
    pinMode(PIN_CLK, INPUT_PULLUP);
    pinMode(PIN_DT,  INPUT_PULLUP);
    pinMode(PIN_SW,  INPUT_PULLUP);
    btn.attach(PIN_SW); btn.interval(8);
    lastAB = readAB();

    // LCD
    setupDisplay();
    enterMenu();

    // náhodné pro efekty
    randomSeed(analogRead(0));
  }

  void handleEncoderScaled() {
    // Soft dekódování – voláme v loop (stačí rychle)
    updateEncoderSoft();

    int8_t d = encDelta;
    encDelta = 0;
    if (d != 0) {
      if (ENC_INVERT) d = -d;
      encAccum += d;
      Serial.print(F("[ENC] rawDelta=")); Serial.print(d); Serial.print(F(" accum=")); Serial.println(encAccum);

      while (encAccum >= ENC_SCALE) {
        encAccum -= ENC_SCALE;
        // krok doprava (DOWN)
        if (ui == UI_MENU) {
          // posun výběru o 1 (lineárně 0..8, bez wrapu či s wrapem – zde s wrapem)
          if (selIndex < 8) selIndex++; else selIndex = 0;
          renderSelection();
          Serial.print(F("[ENC] step RIGHT -> sel=")); Serial.println(selIndex+1);
        }
      }
      while (encAccum <= -ENC_SCALE) {
        encAccum += ENC_SCALE;
        // krok doleva (UP)
        if (ui == UI_MENU) {
          if (selIndex > 0) selIndex--; else selIndex = 8;
          renderSelection();
          Serial.print(F("[ENC] step LEFT  -> sel=")); Serial.println(selIndex+1);
        }
      }
    }

    // tlačítko
    btn.update();
    if (btn.fell()) {
      Serial.println(F("[BTN] PRESS"));
      if (ui == UI_MENU) {
        // přechod do detailu a zároveň zapnout odpovídající efekt
        applyMode(selIndex+1);
        enterDetail();
      } else {
        // návrat do menu
        enterMenu();
      }
    }
  }

  // -------------------- SERIAL PARSER --------------------
  void serialHandleLine(const char* line) {
    // make an uppercase copy for command matching
    char temp[64];
    uint8_t i = 0;
    while (line[i] && i < sizeof(temp)-1) { char c = line[i]; temp[i] = (c >= 'a' && c <= 'z') ? (c - 32) : c; i++; }
    temp[i] = 0;

    if (strcmp(temp, "HELLO") == 0 || strcmp(temp, "?") == 0) {
      Serial.println(F("[DIAG]"));
      Serial.print(F(" ui=")); Serial.print(ui == UI_MENU ? F("MENU") : F("DETAIL"));
      Serial.print(F(" sel=")); Serial.print(selIndex+1);
      Serial.print(F(" mode=")); Serial.print(currentMode);
      Serial.print(F(" bri=")); Serial.print(brightness);
      Serial.print(F(" millis=")); Serial.println(millis());
      return;
    }

    if (strncmp(temp, "MODE", 4) == 0) {
      int n = atoi(line + 4);
      if (n >= 1 && n <= 9) {
        applyMode((uint8_t)n);
        selIndex = (uint8_t)(n - 1);
        if (ui == UI_MENU) {
          renderSelection();
        } else {
          renderDetail(selIndex);
        }
      } else {
        Serial.println(F("[ERR] MODE n (1..9)"));
      }
      return;
    }

    if (strncmp(temp, "BRI", 3) == 0) {
      int n = atoi(line + 3);
      if (n < 0) n = 0; if (n > 255) n = 255;
      applyBrightness((uint8_t)n);
      return;
    }

    if (strcmp(temp, "UP") == 0) {
      if (ui == UI_MENU) {
        if (selIndex > 0) selIndex--; else selIndex = 8;
        renderSelection();
        Serial.print(F("[SIM] UP -> sel=")); Serial.println(selIndex+1);
      } else {
        Serial.println(F("[SIM] UP ignored in DETAIL"));
      }
      return;
    }

    if (strcmp(temp, "DOWN") == 0) {
      if (ui == UI_MENU) {
        if (selIndex < 8) selIndex++; else selIndex = 0;
        renderSelection();
        Serial.print(F("[SIM] DOWN -> sel=")); Serial.println(selIndex+1);
      } else {
        Serial.println(F("[SIM] DOWN ignored in DETAIL"));
      }
      return;
    }

    if (strcmp(temp, "PRESS") == 0) {
      Serial.println(F("[SIM] PRESS"));
      if (ui == UI_MENU) {
        applyMode(selIndex+1);
        enterDetail();
      } else {
        enterMenu();
      }
      return;
    }

    Serial.print(F("[ERR] Unknown: ")); Serial.println(line);
  }

  void serialPoll() {
    while (Serial.available() > 0) {
      char c = (char)Serial.read();
      if (c == '\r') continue;
      if (c == '\n') {
        serialBuf[serialLen] = 0;
        if (serialLen > 0) {
          Serial.print(F("> ")); Serial.println(serialBuf);
          serialHandleLine(serialBuf);
        }
        serialLen = 0;
      } else if (serialLen < sizeof(serialBuf)-1) {
        serialBuf[serialLen++] = c;
      }
    }
  }

  void loop() {
    tNow = millis();

    // UI + enkodér
    handleEncoderScaled();

    // Serial commands
    serialPoll();

    // Efekty běží pořád (aktuální currentMode)
    stepEffect();
  }
