/************************************************************
ZAPOJENÍ (pro děti)
- Žádné dráty mezi deskami, jen USB a Serial Monitor 115200.
- Nejprve nahraj Receiver.ino a zkopíruj jeho MAC.
- Tady ji přepiš do peerMac[] (0xXX, 0xXX, ...), pak nahraj Sender.

Úkoly, pro pochopení kódu:
1) Proč je nutné esp_now_add_peer před esp_now_send?
2) Co znamená stav OK vs NEÚSPĚCH v onDataSent?
3) Doplň „echo“ na Receiveru (pošle zpět potvrzení).
4) Přidej tlačítko: při stisku pošli pevnou zprávu.
5) Ošetři příliš dlouhé vstupy – vytiskni varování.

Poznámka: Kompatibilní s Arduino-ESP32 core 3.x (IDF 5.x).
************************************************************/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>   // pro typ wifi_tx_info_t v novém jádru

static const uint16_t MSG_MAX = 200;
struct Packet { char text[MSG_MAX]; };

// >>> SEM VLOŽ MAC PŘIJÍMAČE (Receiver) podle Serial Monitoru <<<
// Příklad „7C:9E:BD:12:34:56“ -> { 0x7C, 0x9E, 0xBD, 0x12, 0x34, 0x56 }
uint8_t peerMac[6] = { 0x20, 0xE7, 0xC8, 0x68, 0x56, 0xC0 };

esp_now_peer_info_t peerInfo{};

static void printMac(const uint8_t mac[6]) {
  char buf[18];
  snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(buf);
}

// Nový podpis v IDF 5.x. Nečteme pole z `info` (v různých verzích se liší názvy).
static void onDataSent(const wifi_tx_info_t* info, esp_now_send_status_t status) {
  (void)info;  // neřešíme `peer_addr`/`des_addr` → stabilní napříč verzemi
  Serial.print("Send → "); printMac(peerMac);
  Serial.print(" | Stav: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "NEÚSPĚCH");
}

static bool addPeer(const uint8_t mac[6]) {
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, mac, 6);
  peerInfo.channel = 0;         // 0 = aktuální kanál
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

  Serial.println("Sender ready. Napiš řádek a Enter → odešlu ho.");
  printMyMAC();
}

void loop() {
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (!line.length()) return;

    Packet pkt{};                       // naplníme Packet
    line.toCharArray(pkt.text, sizeof(pkt.text));
    if ((int)line.length() >= (int)sizeof(pkt.text)) {
      Serial.println("Pozor: zpráva byla oříznuta na 200 znaků.");
    }

    esp_err_t ok = esp_now_send(peerMac, (uint8_t*)&pkt, sizeof(pkt));
    if (ok != ESP_OK) {
      Serial.print("Chyba esp_now_send: "); Serial.println((int)ok);
    } else {
      Serial.print("Odesláno: "); Serial.println(pkt.text);
    }
  }
}
