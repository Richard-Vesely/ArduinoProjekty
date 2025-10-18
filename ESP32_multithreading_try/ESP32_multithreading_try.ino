/******************************************************
ESP32 IR Remote + NeoPixel (Arduino IDE version)
- Dual-core: Core0 = IR handler (mocked), Core1 = LED shows
- Uses FreeRTOS APIs available in Arduino-ESP32 core

REQUIREMENTS (Arduino IDE):
1) Tools → Board → ESP32 → your ESP32 model (e.g., "ESP32 Dev Module")
2) Library Manager: install "Adafruit NeoPixel" by Adafruit
3) Wiring (same as before):
   NeoPixel DIN -> GPIO 18 (via ~330Ω), 5V -> 5V (external power), GND -> GND
   IR receiver OUT -> GPIO 2, VCC -> 5V, GND -> GND
******************************************************/

#if !defined(ARDUINO_ARCH_ESP32)
#error "This sketch requires ESP32 (Arduino-ESP32 core). Select an ESP32 board in Tools → Board."
#endif

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
// FreeRTOS is already part of Arduino-ESP32; no extra include needed.

#define LED_PIN         18
#define NUMPIXELS       60
#define IR_RECEIVE_PIN  23

// Brightness limits
#define BRIGHT_MIN      5
#define BRIGHT_MAX      255
#define BRIGHT_STEP     10

// Speed limits
#define SPEED_MIN_MS    5
#define SPEED_MAX_MS    500
#define SPEED_STEP_MS   10

// IR button codes (NEC example values; here we simulate presses)
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

struct SharedData {
  uint8_t  currentShow;
  uint8_t  globalBright;
  uint16_t animStep;
  unsigned long stepDelayMs;
  unsigned long fastStepDelayMs;
  unsigned long slowStepDelayMs;
  unsigned long lastIRSignalMs;
  bool          ledUpdatePending;
  unsigned long lastLedUpdateMs;
  unsigned long ledUpdateIntervalMs;

  // Show-specific data
  uint8_t cometPos;
  uint8_t cometTrail[NUMPIXELS];
  uint8_t wipeIndex;
  uint8_t wipeColorIndex;
  uint8_t twinkleVal[NUMPIXELS];
  int8_t  scanPos;
  int8_t  scanDir;
  uint8_t palIdx;

  struct Drop { int8_t pos; uint8_t life; };
  static const uint8_t MAX_DROPS = 4;
  Drop drops[MAX_DROPS];

  SharedData():
    currentShow(1), globalBright(120), animStep(0),
    stepDelayMs(30), fastStepDelayMs(15), slowStepDelayMs(30),
    lastIRSignalMs(0), ledUpdatePending(false), lastLedUpdateMs(0),
    ledUpdateIntervalMs(200), cometPos(0), wipeIndex(0), wipeColorIndex(0),
    scanPos(0), scanDir(1), palIdx(0) {
      memset(cometTrail, 0, sizeof(cometTrail));
      memset(twinkleVal, 0, sizeof(twinkleVal));
      for (uint8_t i = 0; i < MAX_DROPS; i++) drops[i].life = 0;
  }
};

SharedData sharedData;
SemaphoreHandle_t dataMutex;

Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Simple IR edge detector (placeholder)
static bool irSignalActive = false;
static unsigned long irSignalStart = 0;

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
  if (sharedData.globalBright < BRIGHT_MIN) sharedData.globalBright = BRIGHT_MIN;
  if (sharedData.globalBright > BRIGHT_MAX) sharedData.globalBright = BRIGHT_MAX;
  strip.setBrightness(sharedData.globalBright);
}

void clampSpeed() {
  if (sharedData.fastStepDelayMs < SPEED_MIN_MS) sharedData.fastStepDelayMs = SPEED_MIN_MS;
  if (sharedData.fastStepDelayMs > SPEED_MAX_MS) sharedData.fastStepDelayMs = SPEED_MAX_MS;
  if (sharedData.slowStepDelayMs < SPEED_MIN_MS) sharedData.slowStepDelayMs = SPEED_MIN_MS;
  if (sharedData.slowStepDelayMs > SPEED_MAX_MS) sharedData.slowStepDelayMs = SPEED_MAX_MS;
}

void clearStrip() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
}

// ESP32-compatible IR signal detection (placeholder; LOW = activity)
bool readIRSignal() {
  bool irPin = digitalRead(IR_RECEIVE_PIN);
  unsigned long now = millis();
  bool signalDetected = !irPin; // active LOW

  if (signalDetected && !irSignalActive) {
    irSignalActive = true;
    irSignalStart = now;
  } else if (!signalDetected && irSignalActive) {
    irSignalActive = false;
    unsigned long duration = now - irSignalStart;
    if (duration > 10 && duration < 100) {
      return true;
    }
  }
  return false;
}

// Simulate button presses to demonstrate
uint32_t getSimulatedButton() {
  static unsigned long lastButtonTime = 0;
  static uint8_t currentButton = 1;
  unsigned long now = millis();
  if (now - lastButtonTime > 5000) {
    lastButtonTime = now;
    currentButton = (currentButton % 9) + 1;
    switch (currentButton) {
      case 1: return ONE; case 2: return TWO; case 3: return THREE;
      case 4: return FOUR; case 5: return FIVE; case 6: return SIX;
      case 7: return SEVEN; case 8: return EIGHT; case 9: return NINE;
    }
  }
  return 0;
}

const char* buttonName(uint32_t code) {
  if (code == ONE) return "ONE"; if (code == TWO) return "TWO"; if (code == THREE) return "THREE";
  if (code == FOUR) return "FOUR"; if (code == FIVE) return "FIVE"; if (code == SIX) return "SIX";
  if (code == SEVEN) return "SEVEN"; if (code == EIGHT) return "EIGHT"; if (code == NINE) return "NINE";
  if (code == LEFTBTN) return "LEFT"; if (code == RIGHTBTN) return "RIGHT";
  if (code == UPBTN) return "UP"; if (code == DOWNBTN) return "DOWN";
  return "UNKNOWN";
}

// ----- LED Shows -----
void show1_rainbowCycle() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + sharedData.animStep) & 0xFF));
  }
  sharedData.ledUpdatePending = true;
  sharedData.animStep++;
}

void show2_theaterChase() {
  uint8_t offset = sharedData.animStep % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i % 3 == offset) strip.setPixelColor(i, strip.Color(255, 180, 40));
    else strip.setPixelColor(i, 0);
  }
  sharedData.ledUpdatePending = true;
  sharedData.animStep++;
}

void show3_breathing() {
  uint8_t phase = sharedData.animStep & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint8_t r = 0, g = (uint16_t)tri * 2 / 3, b = tri;
  for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, strip.Color(r, g, b));
  sharedData.ledUpdatePending = true;
  sharedData.animStep++;
}

void show4_comet() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (sharedData.cometTrail[i] > 5) sharedData.cometTrail[i] -= 5;
    else sharedData.cometTrail[i] = 0;
    uint8_t v = sharedData.cometTrail[i];
    strip.setPixelColor(i, strip.Color(v, (v * 2) / 3, 10));
  }
  sharedData.cometTrail[sharedData.cometPos] = 255;
  sharedData.ledUpdatePending = true;
  sharedData.cometPos = (sharedData.cometPos + 1) % NUMPIXELS;
  sharedData.animStep++;
}

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
  strip.setPixelColor(sharedData.wipeIndex, basicPalette(sharedData.wipeColorIndex));
  sharedData.ledUpdatePending = true;
  sharedData.wipeIndex++;
  if (sharedData.wipeIndex >= NUMPIXELS) {
    sharedData.wipeIndex = 0;
    sharedData.wipeColorIndex++;
  }
  sharedData.animStep++;
}

void show6_twinkle() {
  if (random(0, 100) < 30) {
    uint16_t p = random(0, NUMPIXELS);
    sharedData.twinkleVal[p] = 180 + random(0, 76);
  }
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (sharedData.twinkleVal[i] > 3) sharedData.twinkleVal[i] -= 3;
    else sharedData.twinkleVal[i] = 0;
    uint8_t v = sharedData.twinkleVal[i];
    strip.setPixelColor(i, strip.Color(v, v, (uint16_t)v * 3 / 2));
  }
  sharedData.ledUpdatePending = true;
  sharedData.animStep++;
}

void show7_scanner() {
  clearStrip();
  strip.setPixelColor(sharedData.scanPos, strip.Color(255, 30, 30));
  int16_t left = sharedData.scanPos - 1;
  int16_t right = sharedData.scanPos + 1;
  if (left >= 0)  strip.setPixelColor(left,  strip.Color(120, 10, 10));
  if (right < NUMPIXELS) strip.setPixelColor(right, strip.Color(120, 10, 10));
  sharedData.ledUpdatePending = true;
  sharedData.scanPos += sharedData.scanDir;
  if (sharedData.scanPos <= 0 || sharedData.scanPos >= (int)NUMPIXELS - 1) sharedData.scanDir = -sharedData.scanDir;
  sharedData.animStep++;
}

void spawnDrop() {
  for (uint8_t i = 0; i < SharedData::MAX_DROPS; i++) {
    if (sharedData.drops[i].life == 0) {
      sharedData.drops[i].pos = random(0, NUMPIXELS);
      sharedData.drops[i].life = 30 + random(0, 40);
      return;
    }
  }
}

void show8_rain() {
  if (random(0, 100) < 25) spawnDrop();

  // Fade background
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    r = (r > 5) ? (r - 5) : 0;
    g = (g > 5) ? (g - 5) : 0;
    b = (b > 5) ? (b - 5) : 0;
    strip.setPixelColor(i, r, g, b);
  }

  // Move drops
  for (uint8_t i = 0; i < SharedData::MAX_DROPS; i++) {
    if (sharedData.drops[i].life) {
      strip.setPixelColor(sharedData.drops[i].pos, strip.Color(40, 0, 200));
      sharedData.drops[i].pos = (sharedData.drops[i].pos + 1) % NUMPIXELS;
      sharedData.drops[i].life--;
    }
  }
  sharedData.ledUpdatePending = true;
  sharedData.animStep++;
}

const uint8_t PAL_SIZE = 5;
uint32_t palette9[PAL_SIZE] = {
  0xFF5500, 0x00FF88, 0x3355FF, 0xFF00AA, 0xFFFF22
};

void show9_palettePulse() {
  uint8_t phase = sharedData.animStep & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint32_t base = palette9[sharedData.palIdx];
  uint8_t br = tri;

  uint8_t r = (base >> 16) & 0xFF;
  uint8_t g = (base >> 8) & 0xFF;
  uint8_t b = base & 0xFF;

  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint8_t local = (br * (uint8_t)((i * 8) % 64 + 192)) / 255;
    strip.setPixelColor(i, (uint16_t)r * local / 255,
                           (uint16_t)g * local / 255,
                           (uint16_t)b * local / 255);
  }
  sharedData.ledUpdatePending = true;
  if ((sharedData.animStep % 180) == 0) sharedData.palIdx = (sharedData.palIdx + 1) % PAL_SIZE;
  sharedData.animStep++;
}

// ----- Task helpers -----
void updateLEDIntervalBasedOnIR() {
  unsigned long now = millis();
  bool irActive = (now - sharedData.lastIRSignalMs) < 750;
  if (irActive) {
    sharedData.ledUpdateIntervalMs = 600;
    sharedData.stepDelayMs = sharedData.slowStepDelayMs;
  } else {
    sharedData.ledUpdateIntervalMs = 10;
    sharedData.stepDelayMs = sharedData.fastStepDelayMs;
  }
}

void updateLEDsIfNeeded() {
  unsigned long now = millis();
  if (sharedData.ledUpdatePending && (now - sharedData.lastLedUpdateMs >= sharedData.ledUpdateIntervalMs)) {
    strip.show();
    sharedData.ledUpdatePending = false;
    sharedData.lastLedUpdateMs = now;
  }
}

// ----- Tasks -----
void IRTask(void* parameter) {
  Serial.println(String("IR Task started on Core ") + xPortGetCoreID());
  Serial.println("Using simplified IR detection (buttons simulated every 5s).");

  for (;;) {
    if (readIRSignal()) {
      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        uint32_t signal = getSimulatedButton(); // replace with real decode for actual remote
        if (signal != 0) {
          sharedData.lastIRSignalMs = millis();
          Serial.print("IR: "); Serial.println(buttonName(signal));

          // Show selection
          if      (signal == ONE)   { sharedData.currentShow = 1; sharedData.animStep = 0; }
          else if (signal == TWO)   { sharedData.currentShow = 2; sharedData.animStep = 0; }
          else if (signal == THREE) { sharedData.currentShow = 3; sharedData.animStep = 0; }
          else if (signal == FOUR)  { sharedData.currentShow = 4; sharedData.animStep = 0; memset(sharedData.cometTrail,0,sizeof(sharedData.cometTrail)); sharedData.cometPos=0; }
          else if (signal == FIVE)  { sharedData.currentShow = 5; sharedData.animStep = 0; sharedData.wipeIndex=0; sharedData.wipeColorIndex=0; }
          else if (signal == SIX)   { sharedData.currentShow = 6; sharedData.animStep = 0; memset(sharedData.twinkleVal,0,sizeof(sharedData.twinkleVal)); }
          else if (signal == SEVEN) { sharedData.currentShow = 7; sharedData.animStep = 0; sharedData.scanPos=0; sharedData.scanDir=1; }
          else if (signal == EIGHT) { sharedData.currentShow = 8; sharedData.animStep = 0; for (uint8_t i=0;i<SharedData::MAX_DROPS;i++){sharedData.drops[i].life=0;} }
          else if (signal == NINE)  { sharedData.currentShow = 9; sharedData.animStep = 0; sharedData.palIdx=0; }

          // Speed
          else if (signal == LEFTBTN) {
            if (sharedData.fastStepDelayMs + SPEED_STEP_MS <= SPEED_MAX_MS) sharedData.fastStepDelayMs += SPEED_STEP_MS;
            if (sharedData.slowStepDelayMs + SPEED_STEP_MS <= SPEED_MAX_MS) sharedData.slowStepDelayMs += SPEED_STEP_MS;
            clampSpeed();
            Serial.printf("Speed slower: fast=%lums slow=%lums\n", sharedData.fastStepDelayMs, sharedData.slowStepDelayMs);
          } else if (signal == RIGHTBTN) {
            if (sharedData.fastStepDelayMs >= SPEED_STEP_MS) sharedData.fastStepDelayMs -= SPEED_STEP_MS;
            if (sharedData.slowStepDelayMs >= SPEED_STEP_MS) sharedData.slowStepDelayMs -= SPEED_STEP_MS;
            clampSpeed();
            Serial.printf("Speed faster: fast=%lums slow=%lums\n", sharedData.fastStepDelayMs, sharedData.slowStepDelayMs);
          }

          // Brightness
          else if (signal == UPBTN) {
            if (sharedData.globalBright + BRIGHT_STEP <= BRIGHT_MAX) sharedData.globalBright += BRIGHT_STEP;
            else sharedData.globalBright = BRIGHT_MAX;
            clampBrightness();
            Serial.printf("Brightness: %u\n", sharedData.globalBright);
          } else if (signal == DOWNBTN) {
            if (sharedData.globalBright >= BRIGHT_STEP + BRIGHT_MIN) sharedData.globalBright -= BRIGHT_STEP;
            else sharedData.globalBright = BRIGHT_MIN;
            clampBrightness();
            Serial.printf("Brightness: %u\n", sharedData.globalBright);
          }
        }
        xSemaphoreGive(dataMutex);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void LEDTask(void* parameter) {
  Serial.println(String("LED Task started on Core ") + xPortGetCoreID());
  unsigned long lastStepMs = 0;

  for (;;) {
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      updateLEDIntervalBasedOnIR();
      updateLEDsIfNeeded();

      unsigned long now = millis();
      if (now - lastStepMs >= sharedData.stepDelayMs) {
        lastStepMs = now;
        switch (sharedData.currentShow) {
          case 1: show1_rainbowCycle();   break;
          case 2: show2_theaterChase();   break;
          case 3: show3_breathing();      break;
          case 4: show4_comet();          break;
          case 5: show5_colorWipeCycle(); break;
          case 6: show6_twinkle();        break;
          case 7: show7_scanner();        break;
          case 8: show8_rain();           break;
          case 9: show9_palettePulse();   break;
          default: sharedData.currentShow = 1;    break;
        }
      }
      xSemaphoreGive(dataMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// ----- Arduino entry points -----
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\nESP32 IR + NeoPixel (Arduino IDE) - 9 shows");

  dataMutex = xSemaphoreCreateMutex();
  if (!dataMutex) {
    Serial.println("ERROR: Failed to create mutex.");
    while (true) { delay(1000); }
  }

  strip.begin();
  strip.setBrightness(sharedData.globalBright);
  strip.show();

  // Robust random seed on ESP32
  uint32_t seed = esp_random();
  randomSeed(seed);

  pinMode(IR_RECEIVE_PIN, INPUT_PULLUP);
  clampSpeed();
  clampBrightness();

  // Create tasks pinned to cores
  xTaskCreatePinnedToCore(IRTask,  "IRTask",  4096, NULL, 2, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(LEDTask, "LEDTask", 8192, NULL, 1, NULL, 1); // Core 1

  Serial.println("Tasks created. IR is simulated; shows auto-cycle every 5s.");
  Serial.println("For a real remote, integrate an IR decoding library and replace getSimulatedButton().");
}

void loop() {
  // Nothing here; work happens in the tasks
  vTaskDelay(pdMS_TO_TICKS(1000));
}
