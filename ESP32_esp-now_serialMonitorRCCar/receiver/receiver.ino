/******************************************************
ZAPOJENÍ (pro děti — RECEIVER = auto, ESP32 + L298N + 2× DC motor)
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

Co udělat:
1) Nahraj tenhle kód do ESP32 na autě a otevři Serial Monitor (115200).
2) Vypíše se „Moje MAC (Receiver) = …“. Tu adresu zkopíruj.
3) Tu MAC vložíš do SENDERU, a pak bude posílat rychlost bezdrátově.

Úkoly, pro pochopení kódu:
1) Proč se posílá jen jedno číslo a ne dvě? Jak bys to rozšířil na levé+pravé kolo?
2) Co dělá „mrtvá zóna“ a proč je dobrá? (Podívej na DEADZONE.)
3) Uprav rozsah z −255..255 na −100..100. Kde přesně to změníš?
4) Přidej rampu rozjezdu (plynulé zvyšování PWM), aby se auto nerozskakovalo.
******************************************************/

#include <WiFi.h>
#include <esp_now.h>

// ====== Nastavení motorů (L298N) ======
#define IN1 18
#define IN2 19
#define ENA 23   // PWM (rychlost A)

#define IN3 5
#define IN4 17
#define ENB 22   // PWM (rychlost B)

// Můžeš změnit podle potřeby (malé hodnoty ignorujeme kvůli tření motoru)
static const int DEADZONE = 30;       // 0..255
static const int SPEED_MIN = -255;
static const int SPEED_MAX =  255;

// Pokud by překladač hlásil, že analogWrite neexistuje, přidej knihovnu:
//   #include <analogWrite.h>

// ====== Datový paket: posíláme 16bit číslo rychlosti oběma motorům ======
struct PacketSpeed {
  int16_t v;   // −255..255
};

// Pomocné funkce pro motor A/B
static void setMotorA(int value) {
  value = constrain(value, SPEED_MIN, SPEED_MAX);

  // mrtvá zóna
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

// ESP-NOW: callback po přijetí dat (IDF 5.x signatura)
static void onDataRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  (void)info;
  if (len < (int)sizeof(PacketSpeed)) return;
  PacketSpeed pkt{};
  memcpy(&pkt, data, sizeof(pkt));

  // Nastav obě kola stejně
  setMotorA(pkt.v);
  setMotorB(pkt.v);

  Serial.print("RX speed = ");
  Serial.println(pkt.v);
}

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

  // ESP-NOW inicializace (STA mód)
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhal!");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready. Cekam na rychlost (−255..255)...");
  printMyMAC();
}

void loop() {
  // vše řeší callback
}
