/******************************************************
ZAPOJENÍ (ESP32 + OLED SSD1306 + 2× potenciometr 10 kΩ)
------------------------------------------------------
ESP32 → OLED SSD1306 (I2C, typicky 128×64, adresa 0x3C):
- 3V3  → VCC   (OLED chce 3,3 V)
- GND  → GND
- GPIO 21 → SDA
- GPIO 22 → SCL

ESP32 → LEVÝ potenciometr (POT1 – d1,v1):
- jeden krajní vývod → 3V3
- druhý krajní vývod → GND
- střední vývod (běžec) → GPIO 34

ESP32 → PRAVÝ potenciometr (POT2 – d2,v2):
- jeden krajní vývod → 3V3
- druhý krajní vývod → GND
- střední vývod (běžec) → GPIO 35

Co program dělá:
1) Čte oba potenciometry (piny 34 a 35, rozsah 0–4095).
2) Každý potenciometr převede na „rychlost“ v rozsahu -255 až +255.
   - střed (~polovina) = 0 (nic)
   - doleva/dolů = záporná hodnota (zpátečka)
   - doprava/nahoru = kladná hodnota (dopředu)
3) Z těchto hodnot udělá:
   d1 = 0 nebo 1   (0 = backward, 1 = forward)
   v1 = 0–255      (absolutní rychlost)
   d2 = 0 nebo 1
   v2 = 0–255
   podle pravidla:
   - 0 ohm   → 0,255 (plná zpátečka)
   - 5k ohm  → 0,0   (nic)
   - 10k ohm → 1,255 (plná dopředu)
4) Sestaví textovou zprávu "d1,v1,d2,v2", např. "0,255,1,128"
5) Zobrazí všechno na OLED displeji (zatím NIC NEPOSÍLÁ přes ESP-NOW).

Úkoly pro pochopení kódu:
1) Najdi v kódu místo, kde se raw hodnota ADC mění na rozsah -255 až 255.
   Jak bys tam přidal „mrtvou zónu“ kolem nuly (např. když je |v| < 10 → 0)?
2) Změň formát zprávy, aby byl bez čárek (např. "d1v1d2v2" → 01255255128).
3) Zobraz na OLED navíc i raw hodnoty z potenciometrů (0–4095).
4) Změň update interval tak, aby displej neblikalo moc rychle.
5) Připrav si, jak bys tuto zprávu poslal přes ESP-NOW na jiné ESP32.

******************************************************/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// Piny potenciometrů
const int PIN_POT1 = 34;  // levý – d1,v1
const int PIN_POT2 = 35;  // pravý – d2,v2

// Pro přepočet 0–4095 → -255..255 použijeme střed kolem 2048
const int ADC_MAX   = 4095;
const int ADC_MID   = 2048;

// Jednoduchá „mrtvá zóna“ kolem nuly (aby to nekmitalo)
const int DEAD_ZONE = 5;   // hodnoty -5..5 bereme jako 0

// Pomocná funkce: z raw ADC 0–4095 udělá signed -255..255
int adcToSignedSpeed(int raw) {
  // posuneme hodnotu kolem středu
  int centered = raw - ADC_MID;   // ~ -2048..+2047

  // použijeme float, ať je to pro děti jednodušší na pochopení
  float norm = (float)centered / (float)ADC_MID; // cca -1.0..+1.0
  float scaled = norm * 255.0f;                  // -255..+255

  int speed = (int)scaled;

  // ořežeme do rozsahu -255..255 (pro jistotu)
  if (speed > 255)  speed = 255;
  if (speed < -255) speed = -255;

  // mrtvá zóna kolem 0
  if (speed > -DEAD_ZONE && speed < DEAD_ZONE) {
    speed = 0;
  }

  return speed;
}

// Z -255..255 udělá (d, v):
// záporná → d=0, v = -speed
// kladná → d=1, v =  speed
// nula   → d=0, v =  0 (nic se nehýbe)
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

  // pojistka
  if (v > 255) v = 255;
}

// Vykreslení na OLED
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

void setup() {
  Serial.begin(115200);

  pinMode(PIN_POT1, INPUT);
  pinMode(PIN_POT2, INPUT);

  Wire.begin(21, 22); // SDA=21, SCL=22

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // když by se OLED nenašel, můžeme jen psát do Serialu
    Serial.println("OLED init failed!");
    for (;;); // zastav program
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("ESP32 + 2x POT");
  display.println("d1,v1,d2,v2 demo");
  display.display();
  delay(1000);
}

void loop() {
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

  // 4) Sestavení zprávy d1,v1,d2,v2
  char msg[20];
  snprintf(msg, sizeof(msg), "%d,%d,%d,%d", d1, v1, d2, v2);

  // 5) Zobrazení na OLED
  drawOLED(raw1, raw2, speed1, speed2, d1, v1, d2, v2, msg);

  // (pro debug můžeš odkomentovat)
  // Serial.println(msg);

  delay(50); // trochu zpomalíme smyčku, aby OLED neblikalo
}
