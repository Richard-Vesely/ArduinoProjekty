/******************************************************
ESP32 IR Remote + NeoPixel (Arduino IDE)
Refactor: queue-based IR→LED handoff, no throttling, shows only.

- Core 0: IR task — decode & push "button pressed" to a FreeRTOS queue
- Core 1: LED task — reads queue, switches shows, renders frames

REQUIREMENTS (Arduino IDE):
1) Tools → Board → ESP32 → your model (e.g., "ESP32 Dev Module")
2) Library Manager: "Adafruit NeoPixel" by Adafruit
3) Library Manager: "IRremote" (by Armin Joachimsmeyer) v4+
4) Wiring:
   NeoPixel DIN -> GPIO 18 (via ~330Ω), 5V -> 5V (external power), GND -> GND
   IR receiver OUT -> GPIO 23, VCC -> 5V, GND -> GND
******************************************************/

#if !defined(ARDUINO_ARCH_ESP32)
#error "This sketch requires ESP32 (Arduino-ESP32 core). Select an ESP32 board in Tools → Board."
#endif

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp>  // Arduino-IRremote v4+

// ------------ HW config ------------
#define LED_PIN         18
#define NUMPIXELS       60
#define IR_RECEIVE_PIN  23

Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ------------ Buttons / events ------------
enum Button : uint8_t {
  BTN_NONE = 0,
  BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9
};

static inline const char* btnName(Button b) {
  switch (b) {
    case BTN_1: return "1"; case BTN_2: return "2"; case BTN_3: return "3";
    case BTN_4: return "4"; case BTN_5: return "5"; case BTN_6: return "6";
    case BTN_7: return "7"; case BTN_8: return "8"; case BTN_9: return "9";
    default: return "NONE";
  }
}

// ---- Map your NEC command bytes (8-bit) to buttons ----
// Use IRrecvDump example to read your remote, then update these:
#define NEC_CMD_1   0x45
#define NEC_CMD_2   0x46
#define NEC_CMD_3   0x47
#define NEC_CMD_4   0x44
#define NEC_CMD_5   0x40
#define NEC_CMD_6   0x43
#define NEC_CMD_7   0x07
#define NEC_CMD_8   0x15
#define NEC_CMD_9   0x09

static Button mapCommandToButton(uint8_t cmd) {
  switch (cmd) {
    case NEC_CMD_1: return BTN_1;
    case NEC_CMD_2: return BTN_2;
    case NEC_CMD_3: return BTN_3;
    case NEC_CMD_4: return BTN_4;
    case NEC_CMD_5: return BTN_5;
    case NEC_CMD_6: return BTN_6;
    case NEC_CMD_7: return BTN_7;
    case NEC_CMD_8: return BTN_8;
    case NEC_CMD_9: return BTN_9;
    default:        return BTN_NONE;
  }
}

// ------------ FreeRTOS queue ------------
QueueHandle_t buttonQueue;                 // carries Button values from IR task to LED task
static const UBaseType_t QUEUE_LEN = 16;  // bigger to avoid drops

// ------------ LED helpers ------------
static inline uint32_t wheel(uint8_t pos) {
  pos = 255 - pos;
  if (pos < 85)       return strip.Color(255 - pos * 3, 0, pos * 3);
  else if (pos < 170) { pos -= 85;  return strip.Color(0, pos * 3, 255 - pos * 3); }
  else                { pos -= 170; return strip.Color(pos * 3, 255 - pos * 3, 0); }
}

static inline void clearStrip() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, 0);
}

// ------------ LED task state (owned exclusively by LED task) ------------
struct LEDState {
  uint8_t  currentShow = 1;
  uint32_t step = 0;

  // show 4: comet
  uint8_t cometPos = 0;
  uint8_t cometTrail[NUMPIXELS];

  // show 5: wipe
  uint16_t wipeIndex = 0;
  uint8_t  wipeColorIndex = 0;

  // show 6: twinkle
  uint8_t twinkleVal[NUMPIXELS];

  // show 7: scanner
  int16_t scanPos = 0;
  int8_t  scanDir = 1;

  // show 8: rain
  struct Drop { int16_t pos; uint8_t life; };
  static const uint8_t MAX_DROPS = 4;
  Drop drops[MAX_DROPS];

  // show 9: palette
  uint8_t palIdx = 0;

  void resetForShow(uint8_t show) {
    step = 0;
    switch (show) {
      case 4:
        memset(cometTrail, 0, sizeof(cometTrail));
        cometPos = 0;
        break;
      case 5:
        wipeIndex = 0; wipeColorIndex = 0;
        break;
      case 6:
        memset(twinkleVal, 0, sizeof(twinkleVal));
        break;
      case 7:
        scanPos = 0; scanDir = 1;
        break;
      case 8:
        for (uint8_t i = 0; i < MAX_DROPS; i++) drops[i].life = 0;
        break;
      case 9:
        palIdx = 0;
        break;
      default: break;
    }
  }
} S;

// ------------ Shows (write to strip; LED task calls strip.show()) ------------
static inline uint32_t basicPalette(uint8_t idx) {
  switch (idx % 6) {
    case 0: return strip.Color(255, 0, 0);
    case 1: return strip.Color(0, 255, 0);
    case 2: return strip.Color(0, 0, 255);
    case 3: return strip.Color(255, 255, 0);
    case 4: return strip.Color(255, 0, 255);
    default:return strip.Color(0, 255, 255);
  }
}

static void show1_rainbowCycle() {
  for (uint16_t i = 0; i < NUMPIXELS; i++)
    strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + (S.step & 0xFF)) & 0xFF));
  S.step++;
}

static void show2_theaterChase() {
  uint8_t offset = S.step % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++)
    strip.setPixelColor(i, (i % 3 == offset) ? strip.Color(255, 180, 40) : 0);
  S.step++;
}

static void show3_breathing() {
  uint8_t phase = S.step & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint8_t r = 0, g = (uint16_t)tri * 2 / 3, b = tri;
  for (uint16_t i = 0; i < NUMPIXELS; i++) strip.setPixelColor(i, strip.Color(r, g, b));
  S.step++;
}

static void show4_comet() {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint8_t &v = S.cometTrail[i];
    v = (v > 5) ? (v - 5) : 0;
    strip.setPixelColor(i, strip.Color(v, (v * 2) / 3, 10));
  }
  S.cometTrail[S.cometPos] = 255;
  S.cometPos = (S.cometPos + 1) % NUMPIXELS;
  S.step++;
}

static void show5_colorWipeCycle() {
  strip.setPixelColor(S.wipeIndex, basicPalette(S.wipeColorIndex));
  S.wipeIndex++;
  if (S.wipeIndex >= NUMPIXELS) { S.wipeIndex = 0; S.wipeColorIndex++; }
  S.step++;
}

static void show6_twinkle() {
  if (random(0, 100) < 30) {
    uint16_t p = random(0, NUMPIXELS);
    S.twinkleVal[p] = 180 + random(0, 76);
  }
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    uint8_t &v = S.twinkleVal[i];
    v = (v > 3) ? (v - 3) : 0;
    strip.setPixelColor(i, strip.Color(v, v, (uint16_t)v * 3 / 2));
  }
  S.step++;
}

static void show7_scanner() {
  clearStrip();
  strip.setPixelColor(S.scanPos, strip.Color(255, 30, 30));
  int16_t L = S.scanPos - 1, R = S.scanPos + 1;
  if (L >= 0) strip.setPixelColor(L, strip.Color(120, 10, 10));
  if (R < NUMPIXELS) strip.setPixelColor(R, strip.Color(120, 10, 10));
  S.scanPos += S.scanDir;
  if (S.scanPos <= 0 || S.scanPos >= (int)NUMPIXELS - 1) S.scanDir = -S.scanDir;
  S.step++;
}

static void spawnDrop() {
  for (uint8_t i = 0; i < LEDState::MAX_DROPS; i++) {
    if (S.drops[i].life == 0) {
      S.drops[i].pos = random(0, NUMPIXELS);
      S.drops[i].life = 30 + random(0, 40);
      return;
    }
  }
}

static void show8_rain() {
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
  for (uint8_t i = 0; i < LEDState::MAX_DROPS; i++) {
    if (S.drops[i].life) {
      strip.setPixelColor(S.drops[i].pos, strip.Color(40, 0, 200));
      S.drops[i].pos = (S.drops[i].pos + 1) % NUMPIXELS;
      S.drops[i].life--;
    }
  }
  S.step++;
}

static const uint8_t PAL_SIZE = 5;
static uint32_t palette9[PAL_SIZE] = {
  0xFF5500, 0x00FF88, 0x3355FF, 0xFF00AA, 0xFFFF22
};

static void show9_palettePulse() {
  uint8_t phase = S.step & 0xFF;
  uint8_t tri = (phase < 128) ? phase * 2 : (255 - phase) * 2;
  uint32_t base = palette9[S.palIdx];
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
  if ((S.step % 180) == 0) S.palIdx = (S.palIdx + 1) % PAL_SIZE;
  S.step++;
}

// ------------ Tasks ------------

// IR task: tight, non-blocking loop with repeat handling
void IRTask(void* parameter) {
  Serial.println(String("IR Task started on Core ") + xPortGetCoreID());
  uint8_t lastCmd = 0;
  unsigned long lastCmdTime = 0;

  for (;;) {
    if (IrReceiver.decode()) {
      auto &d = IrReceiver.decodedIRData;

      if (d.protocol == NEC) {
        bool isRepeat = d.flags & IRDATA_FLAGS_IS_REPEAT;
        uint8_t cmd = d.command;  // 8-bit NEC command

        if (!isRepeat) {
          lastCmd = cmd;
          lastCmdTime = millis();
          Button b = mapCommandToButton(cmd);
          if (b != BTN_NONE) {
            xQueueSend(buttonQueue, &b, 0);   // non-blocking enqueue
          }
        }
        // Optional auto-repeat if holding a key:
        // else {
        //   unsigned long now = millis();
        //   if (lastCmd && (now - lastCmdTime) > 200) {
        //     Button b = mapCommandToButton(lastCmd);
        //     if (b != BTN_NONE) xQueueSend(buttonQueue, &b, 0);
        //     lastCmdTime = now;
        //   }
        // }
      }

      IrReceiver.resume(); // ready for next frame
    }
    // Let scheduler breathe; no blocking delays
    taskYIELD();
  }
}

void LEDTask(void* parameter) {
  Serial.println(String("LED Task started on Core ") + xPortGetCoreID());

  // fixed frame time (no throttling): ~50 FPS
  const TickType_t FRAME = pdMS_TO_TICKS(20);
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    // Process at most one button per frame (non-blocking)
    Button b = BTN_NONE;
    if (xQueueReceive(buttonQueue, &b, 0) == pdTRUE && b != BTN_NONE) {
      uint8_t nextShow = (uint8_t)b; // BTN_1..BTN_9 map to 1..9
      if (nextShow >= 1 && nextShow <= 9) {
        S.currentShow = nextShow;
        S.resetForShow(S.currentShow);
        Serial.printf("LED: switch to show %u\n", S.currentShow);
      }
    }

    // Run one animation step
    switch (S.currentShow) {
      case 1: show1_rainbowCycle();   break;
      case 2: show2_theaterChase();   break;
      case 3: show3_breathing();      break;
      case 4: show4_comet();          break;
      case 5: show5_colorWipeCycle(); break;
      case 6: show6_twinkle();        break;
      case 7: show7_scanner();        break;
      case 8: show8_rain();           break;
      case 9: show9_palettePulse();   break;
      default: S.currentShow = 1;     break;
    }

    strip.show();
    vTaskDelayUntil(&
