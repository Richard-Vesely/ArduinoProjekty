# ESP32 Pin Configuration and Light Show Definitions

## 🔌 **ESP32 Pin Configuration**

### **Pin Definitions (Lines 37-40):**
```cpp
// =================== CONFIGURATION ===================
#define LED_PIN         6      // NeoPixel data pin
#define NUMPIXELS       60     // Number of LEDs in ring
#define IR_RECEIVE_PIN  2      // IR receiver input pin
```

### **NeoPixel Strip Initialization (Line 118):**
```cpp
Adafruit_NeoPixel strip(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
//                       60 LEDs, GPIO 6, GRB color order, 800kHz
```

### **Pin Usage Summary:**
| Pin | Function | Hardware Connection |
|-----|----------|-------------------|
| **GPIO 6** | NeoPixel Data | LED strip DIN pin (through 330Ω resistor) |
| **GPIO 2** | IR Receiver | IR receiver OUT pin |
| **5V** | Power | LED strip 5V and IR receiver VCC |
| **GND** | Ground | LED strip GND and IR receiver GND |

## 🎨 **Light Show Definitions**

### **Show Functions (Lines 230-366):**

#### **1. Rainbow Cycle (Line 230):**
```cpp
void show1_rainbowCycle() {
    for (uint16_t i = 0; i < NUMPIXELS; i++) {
        strip.setPixelColor(i, wheel((i * 256 / NUMPIXELS + sharedData.animStep) & 0xFF));
    }
    sharedData.ledUpdatePending = true;
    sharedData.animStep++;
}
```

#### **2. Theater Chase (Line 238):**
```cpp
void show2_theaterChase() {
    uint8_t offset = sharedData.animStep % 3;
    for (uint16_t i = 0; i < NUMPIXELS; i++) {
        if (i % 3 == offset) strip.setPixelColor(i, strip.Color(255, 180, 40));
        else strip.setPixelColor(i, 0);
    }
    sharedData.ledUpdatePending = true;
    sharedData.animStep++;
}
```

#### **3. Breathing (Line 248):**
```cpp
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
```

#### **4. Comet (Line 259):**
```cpp
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
```

#### **5. Color Wipe Cycle (Line 285):**
```cpp
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
```

#### **6. Twinkle (Line 296):**
```cpp
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
```

#### **7. Scanner (Line 311):**
```cpp
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
```

#### **8. Rain (Line 334):**
```cpp
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
```

#### **9. Palette Pulse (Line 366):**
```cpp
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
```

## 🔄 **Show Selection Logic (Lines 487-498):**

```cpp
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
```

## 🎯 **Key Points:**

### **Pin Configuration:**
- **GPIO 6**: NeoPixel data pin (defined at line 38)
- **GPIO 2**: IR receiver input (defined at line 40)
- **60 LEDs**: Ring size (defined at line 39)

### **Show Functions:**
- **Each show**: Separate function (show1_ to show9_)
- **All shows**: Use `strip.setPixelColor()` to control LEDs
- **Animation**: Controlled by `sharedData.animStep`
- **Updates**: Marked with `sharedData.ledUpdatePending = true`

### **How to Modify:**
- **Change pins**: Modify `#define` statements at top
- **Add new show**: Create `show10_newEffect()` function
- **Modify colors**: Change RGB values in `strip.Color()` calls
- **Change speed**: Adjust timing in each show function
