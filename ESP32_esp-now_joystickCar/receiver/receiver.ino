/******************************************************
ZAPOJENÍ (pro děti — RECEIVER = auto, ESP32 + L298N + 2× DC motor)
-------------------------------------------------------
ESP32          ->  L298N
-----------------------------
GPIO 18 (IN1)  ->  IN1   (směr Motor A – levé kolo)
GPIO 19 (IN2)  ->  IN2
GPIO 23 (ENA)  ->  ENA   (rychlost Motor A — PWM)

GPIO 5  (IN3)  ->  IN3   (směr Motor B – pravé kolo)
GPIO 17 (IN4)  ->  IN4
GPIO 22 (ENB)  ->  ENB   (rychlost Motor B — PWM)

Napájení:
- Motory napájej 6–12 V do 5V/VIN svorky L298N (podle modulu).
- GND L298N MUSÍ být spojena s GND ESP32 (společná zem).
- Sundej jumpery z ENA a ENB, jinak PWM neprojde.

Úkoly, pro pochopení kódu:
1) Uprav DEADZONE pro plynulejší rozjezd. Která hodnota ti vyhovuje?
2) Přidej „brake“ režim: když jsou oba směrové piny HIGH, motor se aktivně brzdí.
3) Doplň soft-ramp (plynulé dojíždění) při změně rychlosti.
4) Loguj do Serialu jen při změně rychlosti (menší spam).
******************************************************/

#include <WiFi.h>
#include <esp_now.h>

// ----- L298N pins -----
#define IN1 18
#define IN2 19
#define ENA 23
#define IN3 5
#define IN4 17
#define ENB 22

static const int DEADZONE = 30;        // 0..255
static const int SPEED_MIN = -255;
static const int SPEED_MAX =  255;

// Starý jednokanálový paket (pro kompatibilitu)
struct PacketSingle {
  int16_t v;
};
// Nový dvoukanálový
struct PacketDual {
  int16_t a;   // levé
  int16_t b;   // pravé
};

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

// IDF 5.x signatura
static void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;

  if (len == (int)sizeof(PacketDual)) {
    PacketDual pkt{};
    memcpy(&pkt, data, sizeof(pkt));
    setMotorA(pkt.a);
    setMotorB(pkt.b);
    Serial.print("RX L="); Serial.print(pkt.a);
    Serial.print(" R=");    Serial.println(pkt.b);
  } else if (len == (int)sizeof(PacketSingle)) {
    PacketSingle pkt{};
    memcpy(&pkt, data, sizeof(pkt));
    setMotorA(pkt.v);
    setMotorB(pkt.v);
    Serial.print("RX single v="); Serial.println(pkt.v);
  } else {
    Serial.print("RX neznama delka: "); Serial.println(len);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT); pinMode(ENB, OUTPUT);
  setMotorA(0); setMotorB(0);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhal!");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready (dual). Cekam na L/R rychlosti...");
  Serial.print("Moje MAC (Receiver) = "); Serial.println(WiFi.macAddress());
}

void loop() { /* vše řeší callback */ }
