#include <WiFi.h>
#include <esp_now.h>

// ====== KONFIGURACE PEERŮ ======

struct Peer {
  const char* name;   // jméno, které zadáš před dvojtečku (např. "vasik")
  uint8_t mac[6];     // MAC adresa daného ESP
};

// TODO: DOPLŇ SI SPRÁVNÉ MAC ADRESY
Peer peers[] = {
  { "vasik",  { 0x88, 0x57, 0x21, 0x79, 0x7A, 0xEC } },
  { "ludvik", { 0x88, 0x57, 0x21, 0x79, 0xCB, 0x34 } },
  { "mara",   { 0x88, 0x57, 0x21, 0x79, 0xE0, 0x38 } }
};

const int NUM_PEERS = sizeof(peers) / sizeof(peers[0]);

// volitelné – tvoje jméno (když bys chtěl později přidávat info o odesílateli do zprávy)
const char* MY_NAME = "richard";

// ====== CALLBACKY ESP-NOW ======

// core 3: wifi_tx_info_t*, nic z toho nemusíme používat
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  (void)info; // aby nebylo warning "unused parameter"

  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

// core 3: esp_now_recv_info*, incomingData, len
void onDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  (void)info; // info teď nevyužíváme

  const int MAX_LEN = 250;
  char buffer[MAX_LEN];

  int copyLen = len;
  if (copyLen > MAX_LEN - 1) {
    copyLen = MAX_LEN - 1;
  }

  memcpy(buffer, incomingData, copyLen);
  buffer[copyLen] = '\0';

  Serial.print("Received message: ");
  Serial.println(buffer);
}

// ====== POMOCNÉ FUNKCE ======

// Najde index peera podle jména (case-insensitive). Když nenajde, vrátí -1.
int findPeerIndexByName(const String& name) {
  for (int i = 0; i < NUM_PEERS; i++) {
    if (name.equalsIgnoreCase(peers[i].name)) {
      return i;
    }
  }
  return -1;
}

// Odešle zprávu konkrétnímu peerovi podle jména
void sendMessageTo(const String& targetName, const String& message) {
  int idx = findPeerIndexByName(targetName);
  if (idx < 0) {
    Serial.print("Unknown user: ");
    Serial.println(targetName);
    Serial.println("Use format: vasik: hello");
    return;
  }

  const Peer& p = peers[idx];

  Serial.print("Sending to ");
  Serial.print(p.name);
  Serial.print(": ");
  Serial.println(message);

  esp_err_t result = esp_now_send(
    p.mac,
    (const uint8_t*)message.c_str(),
    message.length() + 1      // +1 pro '\0'
  );

  if (result != ESP_OK) {
    Serial.print("esp_now_send error, code: ");
    Serial.println(result);
  }
}

// ====== SETUP & LOOP ======

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("Multi-user ESP-NOW chat (ESP32 core 3.x)");
  Serial.println("Type: vasik: ahoj");

  // ESP-NOW potřebuje STA mód
  WiFi.mode(WIFI_STA);

  // Inicializace ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Registrace callbacků
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Přidání všech peerů
  for (int i = 0; i < NUM_PEERS; i++) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peers[i].mac, 6);
    peerInfo.channel = 0;       // 0 = aktuální kanál
    peerInfo.encrypt = false;   // jednoduchá verze bez šifrování

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
      Serial.print("Failed to add peer: ");
      Serial.println(peers[i].name);
    } else {
      Serial.print("Peer added: ");
      Serial.println(peers[i].name);
    }
  }
}

void loop() {
  // Pokud je v Serial Monitoru nový řádek, zpracuj ho
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();

    if (line.length() == 0) return;

    // Očekávám formát: jmeno: zprava
    int colonIndex = line.indexOf(':');
    if (colonIndex < 0) {
      Serial.println("Invalid format. Use: vasik: ahoj");
      return;
    }

    String name = line.substring(0, colonIndex);
    String message = line.substring(colonIndex + 1);

    name.trim();
    message.trim();

    if (name.length() == 0 || message.length() == 0) {
      Serial.println("Invalid format. Use: vasik: ahoj");
      return;
    }

    sendMessageTo(name, message);
  }
}
