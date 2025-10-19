/******************************************************
ZAPOJENÍ (pro děti — SENDER = ovladač)
-------------------------------------------------------
- Není co zapojovat: jen USB do počítače a Serial Monitor (115200).
- Nejprve si z RECEIVERU opiš MAC adresu (viz jeho Serial Monitor).
- Tu MAC adresu vlož sem do pole peerMac[] a teprve potom nahraj SENDER.

Jak to používat:
- Do Serial Monitoru napiš číslo v rozsahu −255 až 255 (např. 180 nebo −120)
  a zmáčkni Enter. Ovladač to bezdrátově pošle autu.
- Kladné = dopředu, záporné = dozadu, 0 = stop.

Úkoly, pro pochopení kódu:
1) Co se stane, když zadáš číslo mimo rozsah? Kde se to ořeže?
2) Přidej rychlé opakované posílání poslední hodnoty 10× za sekundu,
   aby se ztratily výpadky paketů.
3) Přidej kontrolku: pokud posílání selže, vytiskni varování do Serialu.
4) Rozšiř paket na dvě hodnoty (levé/pravé kolo) a uprav Receiver.
******************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>   // kvůli typu wifi_tx_info_t v jádru 3.x (IDF 5.x)

// Stejné schéma dat jako na Receiveru
struct PacketSpeed {
  int16_t v;   // −255..255
};

// >>> SEM VLOŽ MAC PŘIJÍMAČE (Receiver) z jeho Serial Monitoru <<<
// Příklad „7C:9E:BD:12:34:56“ → { 0x7C, 0x9E, 0xBD, 0x12, 0x34, 0x56 }
uint8_t peerMac[6] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x12, 0x34 };

esp_now_peer_info_t peerInfo{};

static void printMac(const uint8_t mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(buf);
}

// Nový podpis v IDF 5.x (nečteme pole z `info`, mění se napříč verzemi)
static void onDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  (void)info;
  Serial.print("Send → "); printMac(peerMac);
  Serial.print(" | Stav: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "NEÚSPĚCH");
}

static bool addPeer(const uint8_t mac[6]) {
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;     // 0 = aktuální kanál
  peerInfo.encrypt = false;
  if (esp_now_is_peer_exist(mac)) return true;
  return esp_now_add_peer(&peerInfo) == ESP_OK;
}

static void printMyMAC() {
  Serial.print("Moje MAC (Sender) = ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhal!");
    while (true) delay(1000);
  }

  esp_now_register_send_cb(onDataSent);

  if (!addPeer(peerMac)) {
    Serial.print("Nepovedlo se přidat peer – zkontroluj MAC: ");
    printMac(peerMac); Serial.println();
    while (true) delay(1000);
  }

  Serial.println("Sender ready. Zadej cislo −255..255 a stiskni Enter.");
  printMyMAC();
}

void loop() {
  if (!Serial.available()) return;

  // Přečti celý řádek
  String line = Serial.readStringUntil('\n');
  line.trim();
  if (!line.length()) return;

  // Parsuj číslo
  long val = line.toInt();
  if (val < -255) val = -255;
  if (val > 255)  val = 255;

  PacketSpeed pkt{};
  pkt.v = (int16_t)val;

  // Odeslání
  esp_err_t ok = esp_now_send(peerMac, reinterpret_cast<uint8_t*>(&pkt), sizeof(pkt));
  if (ok != ESP_OK) {
    Serial.print("Chyba esp_now_send: ");
    Serial.println((int)ok);
  } else {
    Serial.print("Odeslano: ");
    Serial.println(pkt.v);
  }

  // Vyčisti případné zbytky v bufferu
  while (Serial.available()) Serial.read();
}
