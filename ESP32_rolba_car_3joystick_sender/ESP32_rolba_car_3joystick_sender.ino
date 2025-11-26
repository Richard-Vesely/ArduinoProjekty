#include <esp_now.h>
#include <WiFi.h>

#define VRX_PIN 34
#define VRY_PIN 35

// 🟢 MAC adresa přijímače – změň podle svého
uint8_t receiverMac[] = { 0x20, 0xE7, 0xC8, 0x67, 0x30, 0x98 };

void setup() {
  Serial.begin(115200);
  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init error!");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);
}

void loop() {
  int left  = analogRead(VRX_PIN) >> 4;       // 0–255
  int right = (analogRead(VRY_PIN) >> 4);

  // vytvoř 6znakový text, např. "123045"
  char message[7];
  snprintf(message, sizeof(message), "%03d%03d", left, right);

  esp_now_send(receiverMac, (uint8_t *)message, sizeof(message));

  Serial.println(message); // pro kontrolu
}

