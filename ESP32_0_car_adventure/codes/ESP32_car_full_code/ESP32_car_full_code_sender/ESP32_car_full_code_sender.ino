/******************************************************
ZAPOJENÍ (ESP32 + OLED SSD1306 + 2× potenciometr 10 kΩ)
------------------------------------------------------
ESP32 → OLED SSD1306 (I2C, typicky 128×64, adresa 0x3C):
- 3V3   → VCC   (OLED chce 3,3 V)
- GND   → GND
- GPIO 21 → SDA
- GPIO 22 → SCL

ESP32 → LEVÝ potenciometr (POT1 – d1,v1):
- jeden krajní vývod  → 3V3
- druhý krajní vývod  → GND
- střední vývod (běžec) → GPIO 34

ESP32 → PRAVÝ potenciometr (POT2 – d2,v2):
- jeden krajní vývod  → 3V3
- druhý krajní vývod  → GND
- střední vývod (běžec) → GPIO 35

DRUHÉ ESP32 (PŘIJÍMAČ):
- jen zapoj napájení a anténu (deska).
- později na něj nahrajeme kód, který bude ESP-NOW zprávu přijímat.

Co program dělá:
1) Čte oba potenciometry (piny 34 a 35, rozsah 0–4095).
2) Každý potenciometr převede na „rychlost“ v rozsahu -255 až +255.
   - spodní konec potenciometru  → plná zpátečka (cca -255)
   - střed (~5 kΩ / ~2048)      → 0 (nic)
   - horní konec → plná dopředu (cca +255)
3) Z těchto hodnot udělá:
   d1 = 0 nebo 1   (0 = backward, 1 = forward), v1 = 0–255
   d2 = 0 nebo 1,  v2 = 0–255
   podle pravidla:
   - 0 ohm   → 0,255 (plná zpátečka)
   - 5k ohm  → 0,0   (nic)
   - 10k ohm → 1,255 (plná dopředu)
4) Sestaví textovou zprávu "d1,v1,d2,v2" (např. "0,255,1,128")
5) Zobrazí hodnoty i zprávu na OLED displeji.
6) Pošle zprávu přes ESP-NOW na jiné ESP32 (podle zadané MAC adresy).

Poznámka k ESP32 core 3.0:
- Kód používá standardní ESP-NOW API (#include <esp_now.h>, WiFi.mode(WIFI_STA),
  esp_now_init(), esp_now_add_peer(), esp_now_send()), které je kompatibilní s
  Arduino-ESP32 core v3.x (podle migration guide a aktuální dokumentace).
- Wi-Fi musí být spuštěné v režimu STA před esp_now_init().

Úkoly pro pochopení kódu:
1) Přidej „mrtvou zónu“ tak, aby se d1/d2 a v1/v2 změnily jen když |speed| > 20.
2) Změň formát zprávy na bezčárkový (např. "d1v1d2v2" → 02550128) a uprav snprintf.
3) Zvětši SEND_INTERVAL_MS, aby se neposílalo tak často.
4) Přidej do kódu přijímače (druhé ESP32), který zprávu přijme a vypíše ji do Serialu.
5) Zkus si v receiveru zpracovat d1,v1,d2,v2 na řízení dvou motorů přes driver.

******************************************************/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <WiFi.h>
#include <esp_now.h>

// ===== OLED nastavení =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// ===== Piny potenciometrů =====
const int PIN_POT1 = 34;  // levý – d1,v1
const int PIN_POT2 = 35;  // pravý – d2,v2

// Převod 0–4095 → -255..255 se středem
const int ADC_MAX   = 4095;
const int ADC_MID   = 2048;

// Mrtvá zóna kolem nuly (aby to nekmitalo)
const int DEAD_ZONE = 5;

// ESP-NOW: MAC adresa přijímače – ZMĚŇ NA MAC DRUHÉHO ESP32
// MAC zjistíš krátkým sketchem s WiFi.macAddress().
uint8_t receiverMac[] = { 0x20, 0xE7, 0xC8, 0x67, 0x30, 0x98 };

// Interval mezi odesláními (ms)
const unsigned long SEND_INTERVAL_MS = 50;
unsigned long lastSendTime = 0;

// Aktuální zpráva
char msg[20];

// ===== Převod z ADC na -255..255 =====
int adcToSignedSpeed(int raw) {
  int centered = raw - ADC_MID;                // cca -2048..+2047
  float norm   = (float)centered / ADC_MID;    // cca -1.0..+1.0
  float scaled = norm * 255.0f;                // -255..+255
  int speed    = (int)scaled;

  if (speed > 255)  speed = 255;
  if (speed < -255) speed = -255;

  // mrtvá zóna
  if (speed > -DEAD_ZONE && speed < DEAD_ZONE) {
    speed = 0;
  }

  return speed;
}

// ===== Převod -255..255 na (d, v) =====
void speedToDirVal(int speed, int &d, int &v) {
  if (speed > 0) {
    d = 1;
    v = speed;
  } else if (speed < 0) {
    d = 0;
    v = -speed;
  } else {
    d = 0;
    v = 0;
  }
  if (v > 255) v = 255;
}

// ===== Vykreslení na OLED =====
void drawOLED(int raw1, int raw2,
              int speed1, int speed2,
              int d1, int v1, int d2, int v2,
              const char* msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("P1 raw:");
  display.print(raw1);

  display.setCursor(0, 10);
  display.print("P2 raw:");
  display.print(raw2);

  display.setCursor(0, 22);
  display.print("P1 s=");
  display.print(speed1);
  display.print(" d=");
  display.print(d1);
  display.print(" v=");
  display.print(v1);

  display.setCursor(0, 34);
  display.print("P2 s=");
  display.print(speed2);
  display.print(" d=");
  display.print(d2);
  display.print(" v=");
  display.print(v2);

  display.setCursor(0, 48);
  display.print("MSG: ");
  display.print(msg);

  display.display();
}

// ===== ESP-NOW callback po odeslání (volitelné) =====
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  // Můžeš sem přidat debug do Serialu, pokud chceš:
  // Serial.print("ESP-NOW send status: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// ===== Inicializace OLED =====
bool setupOLED() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed!");
    return false;
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 POT -> ESPNOW");
  display.display();
  delay(1000);
  return true;
}

// ===== setup() =====
void setup() {
  Serial.begin(115200);

  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);

  Wire.begin(21, 22);  // I2C piny

  setupOLED();

  // Wi-Fi v režimu stanice (nutné pro ESP-NOW)
  WiFi.mode(WIFI_STA);

  // Inicializace ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    return;
  }

  // Registrace callbacku po odeslání
  esp_now_register_send_cb(onDataSent);

  // Přidání peer (přijímače)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;     // 0 = aktuální Wi-Fi kanál
  peerInfo.encrypt = false; // bez šifrování

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
  }

  // Úvodní obrazovka
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Ready: POT -> d,v");
  display.display();
  delay(500);
}

// ===== loop() =====
void loop() {
  unsigned long now = millis();
  if (now - lastSendTime < SEND_INTERVAL_MS) {
    return;  // ještě neodesíláme, jen čekáme
  }
  lastSendTime = now;

  // 1) Čtení potenciometrů
  int raw1 = analogRead(PIN_POT1);
  int raw2 = analogRead(PIN_POT2);

  // 2) Převod na rychlost -255..255
  int speed1 = adcToSignedSpeed(raw1);
  int speed2 = adcToSignedSpeed(raw2);

  // 3) Převod na (d1,v1) a (d2,v2)
  int d1, v1, d2, v2;
  speedToDirVal(speed1, d1, v1);
  speedToDirVal(speed2, d2, v2);

  // 4) Sestavení zprávy d1,v1,d2,v2 (např. "0,255,1,128")
  snprintf(msg, sizeof(msg), "%d,%d,%d,%d", d1, v1, d2, v2);

  // 5) Zobrazení na OLED
  drawOLED(raw1, raw2, speed1, speed2, d1, v1, d2, v2, msg);

  // 6) Odeslání přes ESP-NOW
  esp_err_t result = esp_now_send(receiverMac, (uint8_t*)msg, strlen(msg));

  // Volitelný debug:
  // Serial.print("Sent: ");
  // Serial.print(msg);
  // Serial.print("  result=");
  // Serial.println(result == ESP_OK ? "OK" : "FAIL");
}
