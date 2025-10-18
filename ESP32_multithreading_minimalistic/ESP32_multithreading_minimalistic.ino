/********************************************************
ESP32 Dual-Core NeoPixel + IR (Minimal, 2 shows, snappy)
- Core 0: IR decode (IRremote)
- Core 1: LED animation (Adafruit_NeoPixel)
- Any IR button toggles between Show 1 and Show 2
********************************************************/
#if !defined(ARDUINO_ARCH_ESP32)
#error "Select an ESP32 board in Tools → Board."
#endif

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <IRremote.hpp> // Library: IRremote by Armin Joachimsmeyer

// ---------------- Hardware config ----------------
#define LED_PIN     18
#define NUMPIXELS   60
#define IR_PIN      27   // <— use a quiet pin for IR input

// ---------------- Globals ----------------
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

SemaphoreHandle_t dataMutex;

struct Shared {
  volatile uint8_t currentShow = 1;      // 1: Rainbow, 2: Theater
  volatile uint8_t brightness  = 120;    // 5..255
  // Animation timing (non-blocking)
  volatile uint16_t stepMs     = 16;     // ~60 FPS base (adjust as you like)
} shared;

// For LED task step timing
static unsigned long lastStepMs = 0;

// ---------------- Helpers ----------------
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

// ---------------- Animations (non-blocking) ----------------
void showRainbowStep(uint16_t step) {
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + step) & 0xFF));
  }
}

void showTheaterStep(uint16_t step) {
  uint8_t offset = step % 3;
  for (uint16_t i = 0; i < NUMPIXELS; i++) {
    if (i % 3 == offset) strip.setPixelColor(i, strip.Color(255, 180, 40));
    else                 strip.setPixelColor(i, 0);
  }
}

// ---------------- Tasks ----------------
void IRTask(void* parameter) {
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK, USE_DEFAULT_FEEDBACK_LED_PIN);
  Serial.printf("IR on pin %d (Core %d)\n", IR_PIN, xPortGetCoreID());

  for (;;) {
    if (IrReceiver.decode()) {
      // Print once so you can see real codes and later map specific buttons
      // Serial.printf("RAW=0x%08lX, prot=%d, cmd=0x%02X\n",
      //   IrReceiver.decodedIRData.decodedRawData,
      //   IrReceiver.decodedIRData.protocol,
      //   IrReceiver.decodedIRData.command);

      // Minimal behavior: ANY valid command toggles the show.
      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        shared.currentShow = (shared.currentShow == 1) ? 2 : 1;
        xSemaphoreGive(dataMutex);
      }

      IrReceiver.resume(); // ready for next frame
    }

    // Optional: quick responsiveness + fair scheduling
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void LEDTask(void* parameter) {
  Serial.printf("LED task (Core %d)\n", xPortGetCoreID());
  uint16_t step = 0;

  for (;;) {
    unsigned long now = millis();
    bool doStep = false;
    uint8_t showLocal;
    uint16_t stepMsLocal;
    uint8_t brightLocal;

    // Copy shared state atomically under mutex
    if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
      showLocal    = shared.currentShow;
      stepMsLocal  = shared.stepMs;
      brightLocal  = shared.brightness;
      xSemaphoreGive(dataMutex);
    }

    if (now - lastStepMs >= stepMsLocal) {
      lastStepMs = now;
      doStep = true;
    }

    if (doStep) {
      strip.setBrightness(brightLocal);

      if (showLocal == 1)      showRainbowStep(step);
      else /* showLocal == 2 */ showTheaterStep(step);

      strip.show();
      step++;
    }

    // Yield a tick; prevents task starvation and keeps IR responsive
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// ---------------- Arduino entry points ----------------
void setup() {
  Serial.begin(115200);
  delay(100);

  strip.begin();
  strip.clear();
  strip.setBrightness(shared.brightness);
  strip.show();

  dataMutex = xSemaphoreCreateMutex();
  if (!dataMutex) {
    Serial.println("Failed to create mutex.");
    while (true) { delay(1000); }
  }

  // Seed RNG (if you want to add random later)
  randomSeed(esp_random());

  // Start tasks on separate cores
  xTaskCreatePinnedToCore(IRTask,  "IRTask",  4096, NULL, 2, NULL, 0); // Core 0
  xTaskCreatePinnedToCore(LEDTask, "LEDTask", 8192, NULL, 1, NULL, 1); // Core 1
}

void loop() {
  // Nothing here; both tasks run under FreeRTOS
  vTaskDelay(pdMS_TO_TICKS(1000));
}
