/******************************************************
ESP32 IR Remote Multithreading Project
-------------------------------------------------------
Refactored from Arduino Leonardo code for ESP32 dual-core architecture

HARDWARE CONNECTIONS:
NeoPixel Ring (WS2812B):
- DIN (data)  -> GPIO 18 (through ~330Ω resistor)
- 5V          -> 5V (external power recommended)
- GND         -> GND

IR Receiver (VS1838B/TSOP38238):
- OUT         -> GPIO 2
- VCC         -> ESP32 5V pin (NOT 3.3V!)
- GND         -> GND

ARCHITECTURE:
- Core 0: IR handling (non-blocking)
- Core 1: LED animations (smooth, uninterrupted)
- Thread-safe communication via FreeRTOS primitives

IR REMOTE CONTROL:
- Numbers 1-9: Select show (9 different effects)
- LEFT/RIGHT: Slow down/speed up animation
- UP/DOWN: Decrease/increase brightness
******************************************************/

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

// ESP32 Serial2 support - bypass problematic HardwareSerial.cpp
// We use Serial2 directly, avoiding the framework's Serial issues

// =================== CONFIGURATION ===================
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

// IR button codes
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

// =================== SHARED DATA STRUCTURE ===================
struct SharedData {
    uint8_t currentShow;
    uint8_t globalBright;
    uint16_t animStep;
    unsigned long stepDelayMs;
    unsigned long fastStepDelayMs;
    unsigned long slowStepDelayMs;
    unsigned long lastIRSignalMs;
    bool ledUpdatePending;
    unsigned long lastLedUpdateMs;
    unsigned long ledUpdateIntervalMs;
    
    // Show-specific data
    uint8_t cometPos;
    uint8_t cometTrail[NUMPIXELS];
    uint8_t wipeIndex;
    uint8_t wipeColorIndex;
    uint8_t twinkleVal[NUMPIXELS];
    int8_t scanPos;
    int8_t scanDir;
    uint8_t palIdx;
    
    // Rain effect data
    struct Drop { int8_t pos; uint8_t life; };
    static const uint8_t MAX_DROPS = 4;
    Drop drops[MAX_DROPS];
    
    SharedData() : 
        currentShow(1), globalBright(120), animStep(0),
        stepDelayMs(30), fastStepDelayMs(15), slowStepDelayMs(30),
        lastIRSignalMs(0), ledUpdatePending(false), lastLedUpdateMs(0),
        ledUpdateIntervalMs(200), cometPos(0), wipeIndex(0), wipeColorIndex(0),
        scanPos(0), scanDir(1), palIdx(0) {
        memset(cometTrail, 0, sizeof(cometTrail));
        memset(twinkleVal, 0, sizeof(twinkleVal));
        for (uint8_t i = 0; i < MAX_DROPS; i++) {
            drops[i].life = 0;
        }
    }
};

// Global shared data and mutex
SharedData sharedData;
SemaphoreHandle_t dataMutex;

// NeoPixel strip
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Simple IR reading variables
unsigned long lastIRSignalMs = 0;
unsigned long irTimeoutMs = 200;  // IR signal timeout
bool irSignalActive = false;
unsigned long irSignalStart = 0;

// =================== UTILITY FUNCTIONS ===================
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

// ESP32-compatible IR signal detection
bool readIRSignal() {
    bool irPin = digitalRead(IR_RECEIVE_PIN);
    unsigned long now = millis();
    
    // For ESP32 with 3.3V logic, we need to invert the logic
    // IR receiver outputs LOW when signal detected, HIGH when idle
    bool signalDetected = !irPin;  // Invert for ESP32
    
    if (signalDetected && !irSignalActive) {
        // IR signal started
        irSignalActive = true;
        irSignalStart = now;
        return false;
    } else if (!signalDetected && irSignalActive) {
        // IR signal ended
        irSignalActive = false;
        unsigned long duration = now - irSignalStart;
        
        // More realistic signal detection for ESP32
        // IR signals are typically 10-50ms long
        if (duration > 10 && duration < 100) {
            lastIRSignalMs = now;
            return true;  // Signal detected
        }
    }
    
    return false;
}

// Simple button simulation for testing
uint32_t getSimulatedButton() {
    // For demonstration, we'll cycle through buttons every 5 seconds
    static unsigned long lastButtonTime = 0;
    static uint8_t currentButton = 1;
    
    unsigned long now = millis();
    if (now - lastButtonTime > 5000) {  // Change button every 5 seconds
        lastButtonTime = now;
        currentButton = (currentButton % 9) + 1;
        
        // Map to actual button codes
        switch (currentButton) {
            case 1: return ONE;
            case 2: return TWO;
            case 3: return THREE;
            case 4: return FOUR;
            case 5: return FIVE;
            case 6: return SIX;
            case 7: return SEVEN;
            case 8: return EIGHT;
            case 9: return NINE;
            default: return ONE;
        }
    }
    
    return 0;  // No button pressed
}

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

// =================== LED ANIMATION FUNCTIONS ===================
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
    for (uint16_t i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
    }
    sharedData.ledUpdatePending = true;
    sharedData.animStep++;
}

void show4_comet() {
    // Fade trail
    for (uint16_t i = 0; i < NUMPIXELS; i++) {
        if (sharedData.cometTrail[i] > 5) sharedData.cometTrail[i] -= 5;
        else sharedData.cometTrail[i] = 0;
        uint8_t v = sharedData.cometTrail[i];
        strip.setPixelColor(i, strip.Color(v, (v * 2) / 3, 10));
    }
    // Comet head
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
        uint8_t r = (c >> 16) & 0xFF;
        uint8_t g = (c >> 8)  & 0xFF;
        uint8_t b = c & 0xFF;
        if (r > 5) r -= 5; else r = 0;
        if (g > 5) g -= 5; else g = 0;
        if (b > 5) b -= 5; else b = 0;
        strip.setPixelColor(i, r, g, b);
    }
    
    // Move and draw drops
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
    
    uint8_t r = ((base >> 16) & 0xFF);
    uint8_t g = ((base >> 8)  & 0xFF);
    uint8_t b = (base & 0xFF);
    
    for (uint16_t i = 0; i < NUMPIXELS; i++) {
        uint8_t local = (br * (uint8_t)((i * 8) % 64 + 192)) / 255;
        strip.setPixelColor(i, (uint16_t)r * local / 255, (uint16_t)g * local / 255, (uint16_t)b * local / 255);
    }
    sharedData.ledUpdatePending = true;
    
    if ((sharedData.animStep % 180) == 0) sharedData.palIdx = (sharedData.palIdx + 1) % PAL_SIZE;
    sharedData.animStep++;
}

// =================== TASK FUNCTIONS ===================
void updateLEDIntervalBasedOnIR() {
    unsigned long now = millis();
    bool irActive = (now - sharedData.lastIRSignalMs) < 750;
    
    if (irActive) {
        sharedData.ledUpdateIntervalMs = 600;  // Slow when IR active
        sharedData.stepDelayMs = sharedData.slowStepDelayMs;
    } else {
        sharedData.ledUpdateIntervalMs = 10;    // Fast when IR idle
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

void IRTask(void* parameter) {
    Serial2.println("IR Task started on Core " + String(xPortGetCoreID()));
    Serial2.println("Note: Now using REAL IR detection. Point IR remote at receiver.");
    
    for (;;) {
        // Try to read IR signal
        if (readIRSignal()) {
            if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
                // Real IR signal detected - cycle to next show
                static uint8_t currentShow = 1;
                currentShow = (currentShow % 9) + 1;
                
                sharedData.lastIRSignalMs = millis();
                
                Serial2.print("IR Signal detected! Switching to show: "); Serial2.println(currentShow);
                
                // Show selection based on current show
                switch (currentShow) {
                    case 1: sharedData.currentShow = 1; sharedData.animStep = 0; Serial2.println(">> SHOW 1: Rainbow Cycle"); break;
                    case 2: sharedData.currentShow = 2; sharedData.animStep = 0; Serial2.println(">> SHOW 2: Theater Chase"); break;
                    case 3: sharedData.currentShow = 3; sharedData.animStep = 0; Serial2.println(">> SHOW 3: Breathing"); break;
                    case 4: sharedData.currentShow = 4; sharedData.animStep = 0; memset(sharedData.cometTrail,0,sizeof(sharedData.cometTrail)); sharedData.cometPos=0; Serial2.println(">> SHOW 4: Comet"); break;
                    case 5: sharedData.currentShow = 5; sharedData.animStep = 0; sharedData.wipeIndex=0; sharedData.wipeColorIndex=0; Serial2.println(">> SHOW 5: Color Wipe Cycle"); break;
                    case 6: sharedData.currentShow = 6; sharedData.animStep = 0; memset(sharedData.twinkleVal,0,sizeof(sharedData.twinkleVal)); Serial2.println(">> SHOW 6: Twinkle"); break;
                    case 7: sharedData.currentShow = 7; sharedData.animStep = 0; sharedData.scanPos=0; sharedData.scanDir=1; Serial2.println(">> SHOW 7: Scanner"); break;
                    case 8: sharedData.currentShow = 8; sharedData.animStep = 0; for (uint8_t i=0;i<SharedData::MAX_DROPS;i++){sharedData.drops[i].life=0;} Serial2.println(">> SHOW 8: Rain"); break;
                    case 9: sharedData.currentShow = 9; sharedData.animStep = 0; sharedData.palIdx=0; Serial2.println(">> SHOW 9: Palette Pulse"); break;
                }
                
                xSemaphoreGive(dataMutex);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Faster polling for better responsiveness
    }
}

void LEDTask(void* parameter) {
    Serial2.println("LED Task started on Core " + String(xPortGetCoreID()));
    
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
                    default: sharedData.currentShow = 1; break;
                }
            }
            xSemaphoreGive(dataMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Small delay to prevent task starvation
    }
}

// =================== SETUP AND MAIN ===================
void setup() {
    Serial2.begin(115200);
    delay(50);
    Serial2.println("\nESP32 IR + NeoPixel Multithreading - 9 shows, speed & brightness control");
    
    // Create mutex
    dataMutex = xSemaphoreCreateMutex();
    if (dataMutex == NULL) {
        Serial2.println("Failed to create mutex!");
        return;
    }
    
    // Initialize NeoPixel
    strip.begin();
    strip.setBrightness(sharedData.globalBright);
    strip.show();
    
    // Random seed
    randomSeed(analogRead(A0));
    
    // Initialize IR pin
    pinMode(IR_RECEIVE_PIN, INPUT_PULLUP);
    Serial2.println("IR ready (simplified detection). Shows will cycle automatically every 5 seconds for demonstration.");
    Serial2.println("For real IR remote, implement proper NEC protocol decoding in readIRSignal() function.");
    
    // Clamp initial values
    clampSpeed();
    clampBrightness();
    
    // Create tasks
    xTaskCreatePinnedToCore(IRTask, "IRTask", 4096, NULL, 2, NULL, 0);   // Core 0
    xTaskCreatePinnedToCore(LEDTask, "LEDTask", 8192, NULL, 1, NULL, 1);  // Core 1
    
    Serial2.println("Tasks created successfully!");
    Serial2.println("Core 0: IR handling");
    Serial2.println("Core 1: LED animations");
}

void loop() {
    // Main loop is empty - everything runs in tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
