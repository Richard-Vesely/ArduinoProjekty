/******************************************************
🪛 Zapojení (pro děti — RECEIVER = auto, ESP32 + L298N + 2× DC motor)
-------------------------------------------------------
ESP32          ->  L298N
-----------------------------
GPIO 18 (IN1)  ->  IN1   (směr Motor A – levé kolo)
GPIO 19 (IN2)  ->  IN2   (směr Motor A)
GPIO 23 (ENA)  ->  ENA   (rychlost Motor A — PWM)

GPIO 5  (IN3)  ->  IN3   (směr Motor B – pravé kolo)
GPIO 17 (IN4)  ->  IN4   (směr Motor B)
GPIO 22 (ENB)  ->  ENB   (rychlost Motor B — PWM)

Napájení:
- 5V (nebo VIN) na L298N -> 6–12 V z baterie pro motory (NE z USB!)
- GND L298N propoj se stejnou GND ESP32 (SPOLEČNÁ ZEM je nutná)
- ENA/ENB: sundej z nich jumpery, jinak PWM z ESP32 nebude fungovat

*******************************************************
🧩 Úkoly, pro pochopení kódu:
1) Jak z ASCII zprávy „d1,v1,d2,v2“ (např. "0,255,1,128") získáme 4 čísla bez String?
2) Jak z d1,v1,d2,v2 dopočítáme rychlost levého a pravého kola -255..255?
3) Jak se z levého/pravého kola dopočítá „forward“ a „turn“?
4) Jak bys upravil/a kód, aby d1,v1 ovládalo přímo Motor A a d2,v2 přímo Motor B
   bez výpočtu forward/turn?
*******************************************************/

#include <WiFi.h>
#include <esp_now.h>

// ====== Piny pro L298N ======
#define IN1 18
#define IN2 19
#define ENA 23   // PWM (rychlost A)

#define IN3 5
#define IN4 17
#define ENB 22   // PWM (rychlost B)

// ====== Nastavení rychlostí ======
static const int DEADZONE  = 30;     // malé hodnoty ignorujeme (0..255)
static const int SPEED_MIN = -255;
static const int SPEED_MAX =  255;

// ====== GLOBÁLNÍ PROMĚNNÉ (aktualizuje se po příjmu zprávy) ======
volatile uint8_t gLeftX  = 0;   // 0..255  (virtuální X osa = „turn“)
volatile uint8_t gRightY = 0;   // 0..255  (virtuální Y-  = „forward“)
volatile bool    gNewData = false;

// ====== Pomocné – nastavení motorů ======
static void setMotorA(int value) {
  value = constrain(value, SPEED_MIN, SPEED_MAX);
  if (abs(value) < DEADZONE) value = 0;

  if (value > 0) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, value);
  } else if (value < 0) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
    analogWrite(ENA, -value);
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);
  }
}

static void setMotorB(int value) {
  value = constrain(value, SPEED_MIN, SPEED_MAX);
  if (abs(value) < DEADZONE) value = 0;

  if (value > 0) {
    digitalWrite(IN3, HIGH);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, value);
  } else if (value < 0) {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, HIGH);
    analogWrite(ENB, -value);
  } else {
    digitalWrite(IN3, LOW);
    digitalWrite(IN4, LOW);
    analogWrite(ENB, 0);
  }
}

// ====== ESP-NOW PŘÍJEM – NOVÝ FORMÁT: "d1,v1,d2,v2" ======
// Příklad zprávy: "0,255,1,128"
// d1,d2 ∈ {0,1}, v1,v2 ∈ <0..255>
// směr: 0 = backward, 1 = forward
// Rychlost levého kola: s1 = (d1==1 ? +v1 : -v1)
// Rychlost pravého kola: s2 = (d2==1 ? +v2 : -v2)
//
// Potom z (s1,s2) dopočítáme:
// forward = (s1 + s2) / 2
// turn    = (s1 - s2) / 2
//
// A z forward/turn spočítáme zpětně virtuální ose gRightY/gLeftX,
// aby zbytek kódu (mixování na motory) mohl zůstat STEJNÝ.
static void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (len <= 0) return;

  // Pomocná funkce: přečti celé číslo z ASCII číslic
  auto parseIntFrom = [&](int &val, int &i) {
    val = 0;
    bool any = false;
    while (i < len && data[i] >= '0' && data[i] <= '9') {
      val = val * 10 + (data[i] - '0');
      any = true;
      i++;
    }
    if (!any) val = 0; // když žádná cifra, bereme 0
  };

  int d1 = 0, v1 = 0, d2 = 0, v2 = 0;
  int i = 0;

  // d1
  parseIntFrom(d1, i);
  while (i < len && data[i] != ',') i++;
  if (i < len) i++; // přeskoč čárku

  // v1
  parseIntFrom(v1, i);
  while (i < len && data[i] != ',') i++;
  if (i < len) i++;

  // d2
  parseIntFrom(d2, i);
  while (i < len && data[i] != ',') i++;
  if (i < len) i++;

  // v2
  parseIntFrom(v2, i);

  // Omezíme do rozumného rozsahu
  d1 = (d1 != 0) ? 1 : 0;
  d2 = (d2 != 0) ? 1 : 0;
  if (v1 < 0) v1 = 0; if (v1 > 255) v1 = 255;
  if (v2 < 0) v2 = 0; if (v2 > 255) v2 = 255;

  // Signed rychlosti kol -255..255
  int s1 = (d1 == 1) ? v1 : -v1;   // levé kolo
  int s2 = (d2 == 1) ? v2 : -v2;   // pravé kolo

  // Z nich dopočítáme forward/turn (stejný mix jako v loopu, jen opačně)
  int forward = (s1 + s2) / 2;   // průměr
  int turn    = (s1 - s2) / 2;   // rozdíl/2

  forward = constrain(forward, SPEED_MIN, SPEED_MAX);
  turn    = constrain(turn,    SPEED_MIN, SPEED_MAX);

  // Převod zpět na "joystick" 0..255
  // forward: -255..255 -> gRightY: 0..255  (mapujeme lineárně)
  int y = map(forward, -255, 255, 0, 255);
  int x = map(turn,    -255, 255, 0, 255);

  // Saturace pro jistotu
  if (y < 0)   y = 0;
  if (y > 255) y = 255;
  if (x < 0)   x = 0;
  if (x > 255) x = 255;

  gRightY = (uint8_t)y;
  gLeftX  = (uint8_t)x;
  gNewData = true;

  // Debug (volitelně):
  /*
  Serial.print("MSG: d1="); Serial.print(d1);
  Serial.print(" v1="); Serial.print(v1);
  Serial.print(" d2="); Serial.print(d2);
  Serial.print(" v2="); Serial.print(v2);
  Serial.print(" | s1="); Serial.print(s1);
  Serial.print(" s2="); Serial.print(s2);
  Serial.print(" | f="); Serial.print(forward);
  Serial.print(" t="); Serial.print(turn);
  Serial.print(" | X="); Serial.print(x);
  Serial.print(" Y="); Serial.println(y);
  */
}

// ====== Info MAC ======
static void printMyMAC() {
  Serial.print("Moje MAC (Receiver) = ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // Piny motorů
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  // Bezpečný stop na startu
  setMotorA(0);
  setMotorB(0);

  // ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhal!");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready. Cekam na 'd1,v1,d2,v2' zpravy...");
  printMyMAC();
}

void loop() {
  if (gNewData) {
    gNewData = false;

    // === 3) podle globálních proměnných ovládej motory ===
    // Mapování: Y- (gRightY) -> dopředná rychlost v rozsahu -255..255
    // 0   -> -255 (plná zpět), 128 ~ 0 (stop), 255 -> +255 (plná vpřed)
    int forward = map((int)gRightY, 0, 255, 255, -255) * -1;
    // Otáčení: X (gLeftX) -> -255..255  (doleva záporně, doprava kladně)
    int turn = map((int)gLeftX, 0, 255, -255, 255);

    // Mix pro diferenciální řízení:
    int vA = constrain(forward + turn, SPEED_MIN, SPEED_MAX); // levé kolo (Motor A)
    int vB = constrain(forward - turn, SPEED_MIN, SPEED_MAX); // pravé kolo (Motor B)

    setMotorA(vA);
    setMotorB(vB);

    // Debug výpis:
    Serial.print("LeftX=");
    Serial.print(gLeftX);
    Serial.print("  RightY-=");
    Serial.print(gRightY);
    Serial.print("  |  vA=");
    Serial.print(vA);
    Serial.print("  vB=");
    Serial.println(vB);
  }

  // Sem můžeš přidat failsafe (když dlouho nic nepřišlo -> stop)
}
