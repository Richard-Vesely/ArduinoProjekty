/*
ESP32(-CAM) 2WD Car — Simple Throttle Joystick (forward/back only)
- SoftAP at 192.168.4.50
- Minimal vertical joystick: up = forward, down = reverse, release = stop
- Software PWM on ENA=2 / ENB=12 (works on any ESP32 core)

WiFi:
  SSID: ESP2WDcar1
  PASS: Laska
  URL : http://192.168.4.50/

L298N wiring (unchanged):
  ENA -> GPIO 2   (Left enable)
  ENB -> GPIO 12  (Right enable)
  gpLb -> GPIO 33 (Left backward)
  gpLf -> GPIO 15 (Left forward)
  gpRb -> GPIO 14 (Right backward)
  gpRf -> GPIO 13 (Right forward)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// -------- Pins (unchanged) --------
#define ENA 2
#define ENB 12
#define gpRb 14
#define gpRf 13
#define gpLb 33
#define gpLf 15

// -------- Wi-Fi SoftAP --------
const char* AP_SSID = "ESP2WDcar1";
const char* AP_PASS = "Laska";
IPAddress localIP(192,168,4,50), gateway(192,168,4,1), subnet(255,255,255,0);

// -------- Web --------
WebServer server(80);

// -------- Software PWM (no LEDC) --------
const uint32_t PWM_FREQ_SOFT = 1200;                  // Hz
const uint32_t PWM_PERIOD_US = 1000000UL / PWM_FREQ_SOFT;
volatile uint8_t dutyL = 0, dutyR = 0;                // 0..255
uint32_t phaseStart = 0;

inline void pwmInit() {
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  digitalWrite(ENA, LOW);
  digitalWrite(ENB, LOW);
  phaseStart = micros();
}
inline void pwmWriteLeft(uint8_t d)  { dutyL = d; }
inline void pwmWriteRight(uint8_t d) { dutyR = d; }

inline void pwmService() {
  uint32_t now = micros();
  uint32_t elapsed = now - phaseStart;
  if (elapsed >= PWM_PERIOD_US) {
    phaseStart += PWM_PERIOD_US * (elapsed / PWM_PERIOD_US);
    elapsed = now - phaseStart;
  }
  uint32_t onL = ((uint32_t)dutyL * PWM_PERIOD_US) / 255;
  uint32_t onR = ((uint32_t)dutyR * PWM_PERIOD_US) / 255;
  digitalWrite(ENA, (elapsed < onL) ? HIGH : LOW);
  digitalWrite(ENB, (elapsed < onR) ? HIGH : LOW);
}

// -------- Helpers --------
static inline float clampf(float v, float lo, float hi){
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

// Set both wheels the same based on throttle t ∈ [-1..1]
void setThrottle(float t) {
  // Direction pins (both sides same)
  if (t > 0) { // forward
    digitalWrite(gpLf, HIGH); digitalWrite(gpLb, LOW);
    digitalWrite(gpRf, HIGH); digitalWrite(gpRb, LOW);
  } else if (t < 0) { // backward
    digitalWrite(gpLf, LOW);  digitalWrite(gpLb, HIGH);
    digitalWrite(gpRf, LOW);  digitalWrite(gpRb, HIGH);
  } else { // stop (coast)
    digitalWrite(gpLf, LOW);  digitalWrite(gpLb, LOW);
    digitalWrite(gpRf, LOW);  digitalWrite(gpRb, LOW);
  }

  uint8_t pwm = (uint8_t)(clampf(fabs(t), 0, 1) * 255);
  pwmWriteLeft(pwm);
  pwmWriteRight(pwm);
}

void motorStop(){
  digitalWrite(gpLf, LOW); digitalWrite(gpLb, LOW);
  digitalWrite(gpRf, LOW); digitalWrite(gpRb, LOW);
  pwmWriteLeft(0); pwmWriteRight(0);
}

// -------- HTTP: /th?y=.. (y in -1..1) --------
void handleThrottle() {
  if (!server.hasArg("y")) { server.send(400, "text/plain", "Missing y"); return; }
  float y = clampf(server.arg("y").toFloat(), -1.0f, 1.0f);
  setThrottle(y);
  server.send(204);
}

// -------- Minimal vertical joystick page --------
const char* PAGE = R"HTML(<!doctype html>
<meta charset="utf-8">
<title>Throttle Joystick</title>
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<style>
  * { box-sizing: border-box; }
  html, body { height: 100%; margin: 0; background: #111; color: #eee; font-family: system-ui, sans-serif; }
  .wrap { height: 100%; display: grid; place-items: center; }
  #pad {
    position: relative;
    width: 240px; height: 420px;           /* tall pad for throttle */
    border-radius: 20px;
    background: #222;
    box-shadow: 0 0 0 2px #555 inset;
    touch-action: none;                     /* crucial on mobile */
  }
  /* center line */
  #pad::after {
    content: '';
    position: absolute; left: 50%; top: 50%;
    transform: translate(-50%, -50%);
    width: 80%; height: 2px; background: rgba(255,255,255,.12);
    pointer-events: none;
  }
  #knob {
    position: absolute;
    width: 120px; height: 120px;
    border-radius: 16px;
    background: #444;
    box-shadow: 0 0 0 2px #777 inset;
    left: 50%; transform: translateX(-50%); /* X fixed centered */
  }
  .read { margin-top: 14px; font-family: ui-monospace, Menlo, Consolas, monospace; font-size: 14px; opacity: .9; text-align:center}
</style>
<div class="wrap">
  <div id="pad"><div id="knob"></div></div>
  <div class="read">throttle: <span id="yv">0.00</span></div>
</div>
<script>
(() => {
  const pad  = document.getElementById('pad');
  const knob = document.getElementById('knob');
  const yv   = document.getElementById('yv');

  let rect, cy, radius, kh;
  let dragging = false;
  const rateHz = 30; let last = 0;

  function measure() {
    rect   = pad.getBoundingClientRect();
    cy     = rect.top + rect.height / 2;
    radius = rect.height * 0.4;      // usable half-throw
    kh     = knob.offsetHeight;
    place(0);                        // center
  }

  function place(ny) { // ny ∈ [-1..1], up positive
    const dy = -ny * radius;
    const top = (rect.height/2 + dy) - kh/2;
    knob.style.top = top + 'px';
  }

  function clamp(v) { return v < -1 ? -1 : v > 1 ? 1 : v; }

  function updateFromPointer(py) {
    const dy = py - cy;
    const lim = radius;
    const s = Math.abs(dy) > lim ? lim/Math.abs(dy) : 1;
    const ny = clamp((-dy * s) / lim);   // up positive
    place(ny);
    yv.textContent = ny.toFixed(2);

    const now = performance.now();
    if (now - last > 1000/rateHz) {
      last = now;
      fetch(`/th?y=${ny.toFixed(3)}`).catch(()=>{});
    }
  }

  pad.addEventListener('pointerdown', e => {
    dragging = true;
    pad.setPointerCapture(e.pointerId);
    measure();
    updateFromPointer(e.clientY);
    e.preventDefault();
  });

  pad.addEventListener('pointermove', e => {
    if (!dragging) return;
    updateFromPointer(e.clientY);
    e.preventDefault();
  });

  pad.addEventListener('pointerup', e => {
    dragging = false;
    place(0);
    yv.textContent = '0.00';
    fetch('/th?y=0').catch(()=>{});
    e.preventDefault();
  });

  window.addEventListener('resize', measure);
  window.addEventListener('orientationchange', measure);
  requestAnimationFrame(measure);
})();
</script>)HTML";

void handleRoot(){ server.send(200, "text/html", PAGE); }

void setup() {
  Serial.begin(115200);
  delay(100);

  // Direction pins
  pinMode(gpLb, OUTPUT);
  pinMode(gpLf, OUTPUT);
  pinMode(gpRb, OUTPUT);
  pinMode(gpRf, OUTPUT);

  // Ensure stop
  digitalWrite(gpLb, LOW);
  digitalWrite(gpLf, LOW);
  digitalWrite(gpRb, LOW);
  digitalWrite(gpRf, LOW);

  // PWM and stop
  pwmInit();
  motorStop();

  // Wi-Fi
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/th", HTTP_GET, handleThrottle);
  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  pwmService();
}
