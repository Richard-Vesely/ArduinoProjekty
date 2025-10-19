/************************************************************
ZAPOJENÍ (pro děti)
- Je to bezdrátové: žádné dráty mezi deskami.
- Každé ESP32 připoj k USB vlastnímu počítači (kvůli napájení a Serial Monitoru).
- Otevři Serial Monitor na 115200 baud.

Úkoly pro pochopení kódu:
1) Co je MAC adresa a proč ji odesílač potřebuje?
2) Proč musíme mít WiFi v režimu WIFI_STA pro ESP-NOW?
3) Zkus změnit MSG_MAX a pošli delší větu – co se stane?
4) Přidej k výpisu čas přijetí pomocí millis().
5) Co se stane, když vypneš přijímač – co píše odesílač?
************************************************************/

#include <WiFi.h>
#include <esp_now.h>

static const uint16_t MSG_MAX = 200;

typedef struct {
  char text[MSG_MAX];
} Packet;

// POZOR: v novém jádře má callback jiný podpis:
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  Packet pkt{};
  int copyLen = min(len, (int)sizeof(pkt));
  memcpy(&pkt, incomingData, copyLen);

  // MAC odesílatele je v info->src_addr
  const uint8_t *mac = info->src_addr;
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  Serial.print("Přijato od "); Serial.print(macStr); Serial.print(": ");
  Serial.println(pkt.text);
}

void printMyMAC() {
  // Jednodušší a přenositelné: vezmeme MAC přes WiFi knihovnu
  Serial.print("Moje MAC (Receiver) = ");
  Serial.println(WiFi.macAddress());
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.mode(WIFI_STA);       // ESP-NOW vyžaduje STA (ne AP)
  WiFi.disconnect(true, true);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init selhal!");
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("Receiver ready. Očekávám zprávy…");
  printMyMAC();
}

void loop() {
  // vše řeší callback
}
