/**************************************************************
ZAPOJENÍ (ESP32 + NeoPixel 8 LED)
---------------------------------------------------------------
LED pásek / NeoPixel (WS2812B / WS2815):
- DIN (data)  -> ESP32 pin D5 (GPIO 5) přes odpor ~330 Ω
- VCC         -> 5V (WS2812B)  / 12V (WS2815) z externího zdroje
- GND         -> GND (společná zem s ESP32!)

DŮLEŽITÉ:
- Vždy PROPOJ GND externího zdroje s GND ESP32.
- Dej mezi + a GND na začátek pásku elektrolytický kondenzátor 470–1000 µF.
- ESP32 dává 3.3 V na datu. Většinou to funguje, ale ideální je LVL shifter (3.3V->5V).
- Nepoužívej 5V/12V přímo z ESP32 – LED napájej zvláštním zdrojem.

JAK SE PŘIPOJIT Z MOBILU
---------------------------------------------------------------
1) Nahraj tento kód do ESP32.
2) Na telefonu se připoj k Wi-Fi:
   SSID: ESP32-LED
   Heslo: ardoremy123
3) Otevři prohlížeč a napiš adresu:  http://192.168.1.100
4) Ovládej tlačítky "Show 1–5", "OFF", posuvníky pro Brightness a Speed.

ÚKOLY, PRO POCHOPENÍ KÓDU (zkus bez nápovědy)
---------------------------------------------------------------
1) Změň počet LED (NUM_LEDS) na hodnotu, kterou opravdu máš.
2) Změň SSID a heslo Wi-Fi v proměnných AP_SSID a AP_PASS.
3) Přidej „Show 6“: vytvoř novou funkci advanceShow6(), která udělá
   třeba „theater chase“ s vlastní barvou, a doplň ji do switch().
4) Uprav HTML stránku (proměnná PAGE_html), aby měla tlačítko „Show 6“.
5) Změň výchozí rychlost (speedMs) a zjisti, jak to ovlivní plynulost.
6) Přidej slider pro volbu barvy (R, G, B) a udělej „Color Wipe“ v dané barvě.
7) Pro WS2815: vysvětli spolužákovi rozdíl v napájení (5V vs 12V) a proč
   je nutná společná zem.
8) Najdi v kódu místo, kde se volá strip.show(); proč jej voláme až po
   nastavení všech LED?
9) Proč nepoužíváme delay()? Kde v kódu je „časování“?
10) Dokážeš udělat, aby „OFF“ pouze vypnulo zobrazování, ale po zapnutí
    se show rozběhla z místa, kde skončila?

**************************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>

// ====== UPRAV PODLE SVÉHO HARDWARE ======
#define LED_PIN     5         // Datový pin do LED (GPIO5 = D5)
#define MAX_LEDS    60         // Maximum number of LEDs supported
#define LED_TYPE_GRB NEO_GRB  // Většina pásků je GRB
#define LED_FREQ     NEO_KHZ800

// Dynamic LED count (configurable via web)
uint16_t NUM_LEDS = 12;        // Default LED count

// Wi-Fi Access Point (změň si!)
const char* AP_SSID = "ESP32-LED";
const char* AP_PASS = "ardoremy123";

// Custom IP configuration for Access Point
IPAddress local_IP(192, 168, 1, 100);    // ESP32 IP address
IPAddress gateway(192, 168, 1, 1);        // Gateway IP
IPAddress subnet(255, 255, 255, 0);       // Subnet mask

// ====== NeoPixel objekt ======
Adafruit_NeoPixel strip(MAX_LEDS, LED_PIN, LED_TYPE_GRB | LED_FREQ);

// ====== Animace / stav ======
enum ShowType {
  SHOW_OFF = 0,
  SHOW_1_RAINBOW,
  SHOW_2_THEATER,
  SHOW_3_COLOR_WIPE,
  SHOW_4_BREATH,
  SHOW_5_SPARKLE,
  SHOW_6_FIRE,
  SHOW_7_WAVE,
  SHOW_8_PULSE,
  SHOW_9_CHASE,
  SHOW_10_STROBE
};

volatile ShowType currentShow = SHOW_1_RAINBOW;
volatile bool isOn = true;

uint8_t globalBrightness = 128;    // 0–255
uint16_t speedMs = 80;             // menší = rychlejší (slower for better web responsiveness)
uint32_t lastStepMs = 0;
uint32_t stepIndex = 0;

// RGB color picker variables (configurable via web)
uint8_t baseR = 255, baseG = 60, baseB = 0;  // Starting color for shows
uint8_t fireR = 255, fireG = 100, fireB = 0; // Fire colors
uint8_t waveR = 0, waveG = 150, waveB = 255; // Wave colors

// ====== Web server ======
WebServer server(80);

// Jednoduchá HTML stránka – vše v jedné proměnné a bez externích souborů
const char PAGE_html[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 NeoPixel Control</title>
<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  padding: 20px;
  color: #333;
}

.container {
  max-width: 800px;
  margin: 0 auto;
  background: rgba(255, 255, 255, 0.95);
  border-radius: 20px;
  box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
  overflow: hidden;
}

.header {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: white;
  padding: 30px;
  text-align: center;
}

.header h1 {
  font-size: 2.5rem;
  font-weight: 300;
  margin-bottom: 10px;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.3);
}

.header .subtitle {
  font-size: 1.1rem;
  opacity: 0.9;
  font-weight: 300;
}

.status-bar {
  background: rgba(255, 255, 255, 0.1);
  padding: 15px 30px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-size: 0.9rem;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-value {
  background: rgba(255, 255, 255, 0.2);
  padding: 4px 12px;
  border-radius: 15px;
  font-weight: 500;
}

.content {
  padding: 30px;
}

.card {
  background: white;
  border-radius: 16px;
  padding: 25px;
  margin-bottom: 20px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.2);
  transition: transform 0.2s ease, box-shadow 0.2s ease;
}

.card:hover {
  transform: translateY(-2px);
  box-shadow: 0 8px 25px rgba(0, 0, 0, 0.12);
}

.card-title {
  font-size: 1.3rem;
  font-weight: 600;
  margin-bottom: 20px;
  color: #2c3e50;
  display: flex;
  align-items: center;
  gap: 10px;
}

.card-title::before {
  content: '';
  width: 4px;
  height: 20px;
  background: linear-gradient(135deg, #667eea, #764ba2);
  border-radius: 2px;
}

.btns {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 15px;
  margin-bottom: 10px;
}

button {
  padding: 15px 20px;
  border: none;
  border-radius: 12px;
  cursor: pointer;
  font-size: 1rem;
  font-weight: 500;
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
  background: linear-gradient(135deg, #f8f9fa, #e9ecef);
  color: #495057;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
}

button:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(0, 0, 0, 0.15);
  background: linear-gradient(135deg, #667eea, #764ba2);
  color: white;
}

button:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2);
}

button.off-btn {
  background: linear-gradient(135deg, #ff6b6b, #ee5a52);
  color: white;
}

button.off-btn:hover {
  background: linear-gradient(135deg, #ff5252, #e53935);
}

button.test-btn {
  background: linear-gradient(135deg, #ffa726, #ff9800);
  color: white;
  grid-column: 1 / -1;
}

button.test-btn:hover {
  background: linear-gradient(135deg, #ff9800, #f57c00);
}

.control-row {
  display: flex;
  align-items: center;
  gap: 20px;
  margin-bottom: 15px;
}

.control-label {
  min-width: 120px;
  font-weight: 500;
  color: #495057;
}

.slider-container {
  flex: 1;
  position: relative;
}

input[type="range"] {
  width: 100%;
  height: 8px;
  border-radius: 4px;
  background: #e9ecef;
  outline: none;
  -webkit-appearance: none;
  appearance: none;
}

input[type="range"]::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: linear-gradient(135deg, #667eea, #764ba2);
  cursor: pointer;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.2);
  transition: transform 0.2s ease;
}

input[type="range"]::-webkit-slider-thumb:hover {
  transform: scale(1.1);
}

input[type="range"]::-moz-range-thumb {
  width: 20px;
  height: 20px;
  border-radius: 50%;
  background: linear-gradient(135deg, #667eea, #764ba2);
  cursor: pointer;
  border: none;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.2);
}

.value-display {
  min-width: 50px;
  text-align: center;
  background: linear-gradient(135deg, #667eea, #764ba2);
  color: white;
  padding: 8px 12px;
  border-radius: 8px;
  font-weight: 600;
  font-size: 0.9rem;
}

.help-text {
  color: #6c757d;
  font-size: 0.9rem;
  font-style: italic;
  margin-top: 10px;
}

.status-indicator {
  position: fixed;
  top: 20px;
  right: 20px;
  background: rgba(255, 255, 255, 0.95);
  padding: 15px 20px;
  border-radius: 12px;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1);
  backdrop-filter: blur(10px);
  z-index: 1000;
  transition: all 0.3s ease;
}

.status-indicator.success {
  background: linear-gradient(135deg, #4caf50, #45a049);
  color: white;
}

.status-indicator.error {
  background: linear-gradient(135deg, #f44336, #d32f2f);
  color: white;
}

.status-indicator.loading {
  background: linear-gradient(135deg, #ff9800, #f57c00);
  color: white;
}

.color-wheel-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 15px;
}

.color-wheel {
  width: 200px;
  height: 200px;
  border-radius: 50%;
  background: conic-gradient(
    #ff0000 0deg,
    #ffff00 60deg,
    #00ff00 120deg,
    #00ffff 180deg,
    #0000ff 240deg,
    #ff00ff 300deg,
    #ff0000 360deg
  );
  position: relative;
  cursor: pointer;
  border: 3px solid #fff;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
  transition: transform 0.2s ease;
}

.color-wheel:hover {
  transform: scale(1.05);
}

.color-wheel::before {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  width: 40px;
  height: 40px;
  background: white;
  border-radius: 50%;
  transform: translate(-50%, -50%);
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.3);
}

.color-wheel::after {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  width: 20px;
  height: 20px;
  background: #333;
  border-radius: 50%;
  transform: translate(-50%, -50%);
  pointer-events: none;
}

.color-preview-large {
  width: 80px;
  height: 80px;
  border-radius: 50%;
  background: rgb(255, 60, 0);
  border: 4px solid #fff;
  box-shadow: 0 4px 15px rgba(0, 0, 0, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 0.8rem;
  color: white;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.5);
  font-weight: 600;
}

.color-info {
  text-align: center;
  font-size: 0.9rem;
  color: #666;
  margin-top: 10px;
}

@media (max-width: 768px) {
  .container {
    margin: 10px;
    border-radius: 16px;
  }
  
  .header {
    padding: 20px;
  }
  
  .header h1 {
    font-size: 2rem;
  }
  
  .content {
    padding: 20px;
  }
  
  .btns {
    grid-template-columns: 1fr;
  }
  
  .control-row {
    flex-direction: column;
    align-items: stretch;
    gap: 10px;
  }
  
  .control-label {
    min-width: auto;
  }
  
  .status-bar {
    flex-direction: column;
    gap: 10px;
    text-align: center;
  }
}
</style>
<div class="container">
  <div class="header">
    <h1>&#9733; ESP32 NeoPixel Control</h1>
    <div class="subtitle">Wireless LED Light Show Controller</div>
  </div>
  
  <div class="status-bar">
    <div class="status-item">
      <span>&#127760; IP Address:</span>
      <span class="status-value">192.168.1.100</span>
    </div>
    <div class="status-item">
      <span>&#128225; Status:</span>
      <span class="status-value" id="status">Ready</span>
    </div>
  </div>

  <div class="content">
    <div class="card">
      <div class="card-title">&#127917; Light Shows</div>
      <div class="btns">
        <button onclick="send('/cmd?show=1')">&#127752; Rainbow</button>
        <button onclick="send('/cmd?show=2')">&#127930; Theater Chase</button>
        <button onclick="send('/cmd?show=3')">&#127912; Color Wipe</button>
        <button onclick="send('/cmd?show=4')">&#128168; Breath</button>
        <button onclick="send('/cmd?show=5')">&#10024; Sparkle</button>
        <button onclick="send('/cmd?show=6')">&#128293; Fire</button>
        <button onclick="send('/cmd?show=7')">&#127754; Wave</button>
        <button onclick="send('/cmd?show=8')">&#128165; Pulse</button>
        <button onclick="send('/cmd?show=9')">&#128293; Chase</button>
        <button onclick="send('/cmd?show=10')">&#9889; Strobe</button>
        <button onclick="send('/cmd?show=0')" class="off-btn">&#128308; OFF</button>
        <button onclick="send('/test')" class="test-btn">&#128300; Test LEDs</button>
      </div>
    </div>

    <div class="card">
      <div class="card-title">&#9881;&#65039; Controls</div>
      <div class="control-row">
        <div class="control-label">&#128161; Brightness</div>
        <div class="slider-container">
          <input id="br" type="range" min="0" max="255" value="128" oninput="debounceSet('/set?brightness='+this.value)">
        </div>
        <div class="value-display" id="brv">128</div>
      </div>
      <div class="help-text">0 = Dark, 255 = Maximum brightness</div>
      
      <div class="control-row">
        <div class="control-label">&#9889; Speed</div>
        <div class="slider-container">
          <input id="sp" type="range" min="5" max="200" value="80" oninput="debounceSet('/speed?ms='+this.value)">
        </div>
        <div class="value-display" id="spv">80</div>
      </div>
      <div class="help-text">Lower number = Faster animation</div>
      
      <div class="control-row">
        <div class="control-label">&#128161; LED Count</div>
        <div class="slider-container">
          <input id="leds" type="range" min="1" max="60" value="12" oninput="debounceSet('/leds?count='+this.value)">
        </div>
        <div class="value-display" id="ledsv">12</div>
      </div>
      <div class="help-text">Number of LEDs in your strip (1-60)</div>
    </div>

    <div class="card">
      <div class="card-title">&#127912; Color Wheel</div>
      <div class="color-wheel-container">
        <div class="color-wheel" id="colorWheel" onclick="handleColorWheelClick(event)"></div>
        <div class="color-preview-large" id="colorPreviewLarge">
          <div id="colorText">R255<br>G60<br>B0</div>
        </div>
        <div class="color-info">
          Click on the color wheel to select a color<br>
          <small>Affects: Theater, Breath, Chase, Strobe, Pulse</small>
        </div>
      </div>
    </div>
  </div>
</div>

<script>
const br = document.getElementById('br'), brv=document.getElementById('brv');
const sp = document.getElementById('sp'), spv=document.getElementById('spv');
const leds = document.getElementById('leds'), ledsv=document.getElementById('ledsv');

br.addEventListener('input', ()=> brv.textContent = br.value);
sp.addEventListener('input', ()=> spv.textContent = sp.value);
leds.addEventListener('input', ()=> ledsv.textContent = leds.value);

// Color wheel functionality
let currentColor = {r: 255, g: 60, b: 0};

function handleColorWheelClick(event) {
  const wheel = document.getElementById('colorWheel');
  const rect = wheel.getBoundingClientRect();
  const centerX = rect.left + rect.width / 2;
  const centerY = rect.top + rect.height / 2;
  
  const x = event.clientX - centerX;
  const y = event.clientY - centerY;
  
  const angle = Math.atan2(y, x) * (180 / Math.PI);
  const normalizedAngle = (angle + 360) % 360;
  
  const distance = Math.sqrt(x * x + y * y);
  const maxDistance = rect.width / 2 - 30; // Account for center circle
  
  if (distance > maxDistance) return; // Click outside wheel
  
  // Convert angle to RGB
  const hue = normalizedAngle;
  const saturation = Math.min(distance / maxDistance, 1);
  const lightness = 0.5;
  
  const rgb = hslToRgb(hue / 360, saturation, lightness);
  
  currentColor = {r: rgb[0], g: rgb[1], b: rgb[2]};
  updateColorDisplay();
  updateColorOnServer();
}

function hslToRgb(h, s, l) {
  let r, g, b;
  
  if (s === 0) {
    r = g = b = l; // achromatic
  } else {
    const hue2rgb = (p, q, t) => {
      if (t < 0) t += 1;
      if (t > 1) t -= 1;
      if (t < 1/6) return p + (q - p) * 6 * t;
      if (t < 1/2) return q;
      if (t < 2/3) return p + (q - p) * (2/3 - t) * 6;
      return p;
    };
    
    const q = l < 0.5 ? l * (1 + s) : l + s - l * s;
    const p = 2 * l - q;
    r = hue2rgb(p, q, h + 1/3);
    g = hue2rgb(p, q, h);
    b = hue2rgb(p, q, h - 1/3);
  }
  
  return [Math.round(r * 255), Math.round(g * 255), Math.round(b * 255)];
}

function updateColorDisplay() {
  const preview = document.getElementById('colorPreviewLarge');
  const text = document.getElementById('colorText');
  
  preview.style.background = `rgb(${currentColor.r}, ${currentColor.g}, ${currentColor.b})`;
  text.innerHTML = `R${currentColor.r}<br>G${currentColor.g}<br>B${currentColor.b}`;
}

function updateColorOnServer() {
  if(t2) clearTimeout(t2);
  t2 = setTimeout(()=>send(`/color?r=${currentColor.r}&g=${currentColor.g}&b=${currentColor.b}`), 200);
}

function send(url){ 
  const statusEl = document.getElementById('status');
  statusEl.textContent = 'Sending...';
  statusEl.className = 'status-value loading';
  
  fetch(url)
    .then(response => response.text())
    .then(data => {
      console.log('Response:', data);
      statusEl.textContent = data;
      statusEl.className = 'status-value success';
      setTimeout(() => {
        statusEl.textContent = 'Ready';
        statusEl.className = 'status-value';
      }, 1500);
    })
    .catch(error => {
      console.error('Error:', error);
      statusEl.textContent = 'Error!';
      statusEl.className = 'status-value error';
      setTimeout(() => {
        statusEl.textContent = 'Ready';
        statusEl.className = 'status-value';
      }, 2000);
    }); 
}

let t1=null, t2=null;
function debounceSet(url){
  if(t1) clearTimeout(t1);
  t1 = setTimeout(()=>send(url), 120);
}
</script>
</html>
)HTML";

// ---------- Pomocné funkce pro barvy ----------
uint32_t colorWheel(byte pos) {
  pos = 255 - pos;
  if(pos < 85) {
    return strip.Color(255 - pos * 3, 0, pos * 3);
  }
  if(pos < 170) {
    pos -= 85;
    return strip.Color(0, pos * 3, 255 - pos * 3);
  }
  pos -= 170;
  return strip.Color(pos * 3, 255 - pos * 3, 0);
}

void clearStrip() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, 0);
}

// Quick LED test without blocking delays
void quickLEDTest() {
  Serial.println("Quick LED test - RED");
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(255, 0, 0));
  }
  strip.show();
  delay(50); // Very short delay
  
  Serial.println("Quick LED test - GREEN");
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 255, 0));
  }
  strip.show();
  delay(50);
  
  Serial.println("Quick LED test - BLUE");
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 255));
  }
  strip.show();
  delay(50);
  
  Serial.println("Quick LED test - OFF");
  clearStrip();
  strip.show();
  
  Serial.println("Quick LED test complete!");
}

// Non-blocking LED test for web interface
void testLEDStrip() {
  Serial.println("Manual LED test requested");
  quickLEDTest();
}

// ---------- Animace: jeden "krok" každé volání ----------
void advanceRainbow() {
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, colorWheel((i * 256 / NUM_LEDS + stepIndex) & 255));
  }
}

void advanceTheater() {
  // 3 "běžící" tečky v základní barvě
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    if ((i + stepIndex) % 3 == 0) strip.setPixelColor(i, strip.Color(baseR, baseG, baseB));
    else strip.setPixelColor(i, 0);
  }
}

void advanceColorWipe() {
  // postupně plní pásek barvou (kruh)
  uint16_t idx = stepIndex % (NUM_LEDS + 1);
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, (i < idx) ? strip.Color(0, 120, 255) : 0);
  }
}

void advanceBreath() {
  // hladké "dýchání" jasu v barvě baseR,G,B
  float phase = (stepIndex % 256) / 255.0f;         // 0..1
  float breath = 0.1f + 0.9f * (0.5f - 0.5f * cos(phase * 2 * 3.14159f)); // 0.1..1.0
  uint8_t r = (uint8_t)(baseR * breath);
  uint8_t g = (uint8_t)(baseG * breath);
  uint8_t b = (uint8_t)(baseB * breath);
  for (uint16_t i = 0; i < NUM_LEDS; i++) strip.setPixelColor(i, strip.Color(r, g, b));
}

void advanceSparkle() {
  // náhodné jiskry na tmavém pozadí
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // pomalu zhasínej (fade)
    uint32_t c = strip.getPixelColor(i);
    uint8_t r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    r = (r > 8) ? r - 8 : 0; g = (g > 8) ? g - 8 : 0; b = (b > 8) ? b - 8 : 0;
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  // rozsvitíme pár náhodných pixelů
  for (uint8_t k = 0; k < 2; k++) {
    uint16_t i = random(NUM_LEDS);
    strip.setPixelColor(i, strip.Color(255, 255, 255));
  }
}

// ---------- NEW ANIMATIONS ----------
void advanceFire() {
  // Fire effect with flickering flames
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    // Cooler colors at bottom, hotter at top
    uint8_t heat = ((i * 255) / NUM_LEDS + stepIndex) % 256;
    uint8_t r, g, b;
    
    if (heat < 85) {
      r = heat * 3;
      g = 0;
      b = 0;
    } else if (heat < 170) {
      r = 255;
      g = (heat - 85) * 3;
      b = 0;
    } else {
      r = 255;
      g = 255;
      b = (heat - 170) * 3;
    }
    
    // Add flickering
    uint8_t flicker = random(0, 50);
    r = (r > flicker) ? r - flicker : 0;
    g = (g > flicker) ? g - flicker : 0;
    b = (b > flicker) ? b - flicker : 0;
    
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void advanceWave() {
  // Wave effect with flowing colors
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float wave = sin((i * 2 * 3.14159 / NUM_LEDS) + (stepIndex * 0.1));
    float intensity = (wave + 1.0) / 2.0; // Normalize to 0-1
    
    uint8_t r = (uint8_t)(waveR * intensity);
    uint8_t g = (uint8_t)(waveG * intensity);
    uint8_t b = (uint8_t)(waveB * intensity);
    
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void advancePulse() {
  // Pulsing effect with expanding/contracting rings
  uint16_t center = NUM_LEDS / 2;
  float pulse = sin(stepIndex * 0.2) * 0.5 + 0.5; // 0-1
  
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    float distance = abs(i - center);
    float intensity = max(0.0, 1.0 - (distance / (NUM_LEDS * pulse * 0.3)));
    
    uint8_t r = (uint8_t)(baseR * intensity);
    uint8_t g = (uint8_t)(baseG * intensity);
    uint8_t b = (uint8_t)(baseB * intensity);
    
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
}

void advanceChase() {
  // Chasing dots effect
  uint8_t numDots = 3;
  uint16_t spacing = NUM_LEDS / numDots;
  
  for (uint16_t i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  
  for (uint8_t d = 0; d < numDots; d++) {
    uint16_t pos = (stepIndex + d * spacing) % NUM_LEDS;
    strip.setPixelColor(pos, strip.Color(baseR, baseG, baseB));
  }
}

void advanceStrobe() {
  // Strobe effect with rapid on/off
  bool strobeOn = (stepIndex % 4) < 2; // 50% duty cycle
  
  if (strobeOn) {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, strip.Color(baseR, baseG, baseB));
    }
  } else {
    for (uint16_t i = 0; i < NUM_LEDS; i++) {
      strip.setPixelColor(i, 0);
    }
  }
}

// ---------- Aplikační logika ----------
void setShow(ShowType s) {
  currentShow = s;
  isOn = (s != SHOW_OFF);
  stepIndex = 0;
  lastStepMs = millis(); // Reset timing to start immediately
  
  Serial.println("Setting show to: " + String(s) + " (isOn: " + String(isOn) + ")");
  
  if (!isOn) {
    Serial.println("Turning off LEDs");
    clearStrip();
    strip.show();
  } else {
    Serial.println("Starting LED animation...");
    // Force immediate update when switching shows
    switch (currentShow) {
      case SHOW_1_RAINBOW:   advanceRainbow();   Serial.println("Rainbow started"); break;
      case SHOW_2_THEATER:   advanceTheater();   Serial.println("Theater started"); break;
      case SHOW_3_COLOR_WIPE:advanceColorWipe(); Serial.println("Color Wipe started"); break;
      case SHOW_4_BREATH:    advanceBreath();    Serial.println("Breath started"); break;
      case SHOW_5_SPARKLE:   advanceSparkle();   Serial.println("Sparkle started"); break;
      case SHOW_6_FIRE:      advanceFire();      Serial.println("Fire started"); break;
      case SHOW_7_WAVE:      advanceWave();      Serial.println("Wave started"); break;
      case SHOW_8_PULSE:     advancePulse();     Serial.println("Pulse started"); break;
      case SHOW_9_CHASE:     advanceChase();     Serial.println("Chase started"); break;
      case SHOW_10_STROBE:   advanceStrobe();    Serial.println("Strobe started"); break;
      default: break;
    }
    strip.show();
    Serial.println("LED strip updated");
  }
}

void handleRoot() {
  server.send_P(200, "text/html", PAGE_html);
}

void handleCmd() {
  if (!server.hasArg("show")) { server.send(400, "text/plain", "Missing ?show="); return; }
  int s = server.arg("show").toInt();
  if (s < 0 || s > 10) s = 0; // Updated to support shows 0-10
  setShow((ShowType)s);
  server.send(200, "text/plain", "Show set to: " + String(s));
}

void handleSet() {
  if (server.hasArg("brightness")) {
    int b = server.arg("brightness").toInt();
    if (b < 0) b = 0; if (b > 255) b = 255;
    globalBrightness = (uint8_t)b;
    strip.setBrightness(globalBrightness);
  }
  server.send(200, "text/plain", "OK");
}

void handleSpeed() {
  if (!server.hasArg("ms")) { server.send(400, "text/plain", "Missing ?ms="); return; }
  int ms = server.arg("ms").toInt();
  if (ms < 5) ms = 5; if (ms > 1000) ms = 1000;
  speedMs = (uint16_t)ms;
  server.send(200, "text/plain", "OK");
}

void handleTest() {
  Serial.println("Manual LED test requested via web");
  testLEDStrip();
  server.send(200, "text/plain", "LED test completed - check Serial monitor");
}

void handleStatus() {
  String status = "ESP32 Status:\n";
  status += "WiFi: " + String(WiFi.softAPIP().toString()) + "\n";
  status += "Clients: " + String(WiFi.softAPgetStationNum()) + "\n";
  status += "LEDs: " + String(NUM_LEDS) + "\n";
  status += "Show: " + String(currentShow) + "\n";
  status += "Brightness: " + String(globalBrightness) + "\n";
  status += "Speed: " + String(speedMs) + "ms\n";
  status += "Base Color: R" + String(baseR) + " G" + String(baseG) + " B" + String(baseB) + "\n";
  server.send(200, "text/plain", status);
}

void handleLEDCount() {
  if (!server.hasArg("count")) { server.send(400, "text/plain", "Missing ?count="); return; }
  int count = server.arg("count").toInt();
  if (count < 1) count = 1;
  if (count > 60) count = 60; // Updated to support up to 60 LEDs
  
  NUM_LEDS = (uint16_t)count;
  Serial.println("LED count set to: " + String(NUM_LEDS));
  
  // Clear and restart current show with new LED count
  clearStrip();
  strip.show();
  setShow(currentShow);
  
  server.send(200, "text/plain", "LED count set to: " + String(NUM_LEDS));
}

void handleColor() {
  if (server.hasArg("r")) {
    int r = server.arg("r").toInt();
    if (r < 0) r = 0; if (r > 255) r = 255;
    baseR = (uint8_t)r;
  }
  if (server.hasArg("g")) {
    int g = server.arg("g").toInt();
    if (g < 0) g = 0; if (g > 255) g = 255;
    baseG = (uint8_t)g;
  }
  if (server.hasArg("b")) {
    int b = server.arg("b").toInt();
    if (b < 0) b = 0; if (b > 255) b = 255;
    baseB = (uint8_t)b;
  }
  
  Serial.println("Base color set to: R" + String(baseR) + " G" + String(baseG) + " B" + String(baseB));
  
  // Update current show with new color
  if (isOn) {
    setShow(currentShow);
  }
  
  server.send(200, "text/plain", "Color set to: R" + String(baseR) + " G" + String(baseG) + " B" + String(baseB));
}

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  Serial.println("ESP32 NeoPixel Lightshow Starting...");
  
  // PRIORITY 1: Start WiFi AP IMMEDIATELY for fastest connection
  Serial.println("Setting up WiFi Access Point (PRIORITY)...");
  WiFi.mode(WIFI_AP);
  
  // Optimize WiFi AP settings for faster connection
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS, 1, 0, 4); // Channel 1, no hidden, max 4 clients
  
  // Start web server IMMEDIATELY after WiFi
  server.on("/", handleRoot);
  server.on("/cmd", handleCmd);     // /cmd?show=0..10
  server.on("/set", handleSet);     // /set?brightness=0..255
  server.on("/speed", handleSpeed); // /speed?ms=5..1000
  server.on("/test", handleTest);   // /test - manual LED test
  server.on("/status", handleStatus); // /status - system status
  server.on("/leds", handleLEDCount); // /leds?count=1..50
  server.on("/color", handleColor);   // /color?r=0..255&g=0..255&b=0..255
  server.begin();
  
  Serial.println("WiFi AP started! SSID: " + String(AP_SSID));
  Serial.println("IP: " + WiFi.softAPIP().toString());
  Serial.println("Web server ready - page available immediately!");
  
  // PRIORITY 2: Initialize LED strip (after web server is ready)
  Serial.println("Initializing LED strip...");
  strip.begin();
  delay(50); // Reduced delay
  
  strip.setBrightness(globalBrightness);
  Serial.println("LED strip initialized with brightness: " + String(globalBrightness));
  
  // Clear strip and show initial state
  clearStrip();
  strip.show();
  delay(50); // Reduced delay
  
  // PRIORITY 3: Quick LED test (non-blocking)
  quickLEDTest();
  
  // PRIORITY 4: Start Show 1 (Rainbow) automatically
  Serial.println("Starting Show 1 (Rainbow) automatically...");
  setShow(SHOW_1_RAINBOW);

  // Random seed pro efekty
  randomSeed(esp_random());
  Serial.println("Setup complete! Web page ready, Rainbow show running.");
}

void loop() {
  // Prioritize web server - handle multiple clients per loop
  for (int i = 0; i < 3; i++) {
    server.handleClient();
    if (server.hasArg("show") || server.hasArg("brightness") || server.hasArg("ms")) {
      // If there's a web request, handle it immediately
      break;
    }
  }

  // LED animation timing (less frequent to give web server priority)
  uint32_t now = millis();
  if (now - lastStepMs >= speedMs) {
    lastStepMs = now;

    if (isOn) {
      switch (currentShow) {
        case SHOW_1_RAINBOW:   advanceRainbow();   break;
        case SHOW_2_THEATER:   advanceTheater();   break;
        case SHOW_3_COLOR_WIPE:advanceColorWipe(); break;
        case SHOW_4_BREATH:    advanceBreath();    break;
        case SHOW_5_SPARKLE:   advanceSparkle();   break;
        case SHOW_6_FIRE:      advanceFire();      break;
        case SHOW_7_WAVE:      advanceWave();      break;
        case SHOW_8_PULSE:     advancePulse();     break;
        case SHOW_9_CHASE:     advanceChase();     break;
        case SHOW_10_STROBE:   advanceStrobe();    break;
        default: break;
      }
      strip.show();
      stepIndex++;
    }
  }
  
  // Small delay to prevent overwhelming the system
  delay(1);
}
