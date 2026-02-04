/******************************************************
SENDER (kids version)
Type a message in Serial Monitor → sends it via ESP-NOW
******************************************************/

#include <WiFi.h>
#include <esp_now.h>

// ===== RECEIVER MAC ADDRESS (already updated!) =====
uint8_t receiverMac[] = { 0xC0, 0xCD, 0xD6, 0x8D, 0xE5, 0x80 };

// ===== ESP-NOW send callback (core v3.x) =====
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("=== ESP-NOW SENDER ===");
  Serial.println("Type a message and press ENTER.");

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }

  esp_now_register_send_cb(onDataSent);

  // Add receiver peer
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, receiverMac, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("Could not add peer!");
    while (true);
  }
}

void loop() {
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    msg.trim();

    if (msg.length() > 0) {
      esp_now_send(receiverMac, (uint8_t*)msg.c_str(), msg.length());
      Serial.print("Sent: ");
      Serial.println(msg);
    }
  }
}
