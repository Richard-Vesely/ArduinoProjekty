/******************************************************
ESP-NOW RECEIVER – ESP32-CAM VERSION
Receives text messages and prints them to Serial Monitor.
******************************************************/

#include <WiFi.h>
#include <esp_now.h>

// ===== receive callback (core v3.x) =====
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  Serial.print("From MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", info->src_addr[i]);
    if (i < 5) Serial.print(":");
  }

  Serial.print(" | Message: ");
  for (int i = 0; i < len; i++) {
    Serial.print((char)data[i]);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(300);

  Serial.println("\n=== ESP32-CAM ESP-NOW RECEIVER ===");
  Serial.print("My MAC address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Give this MAC to the sender!");

  // ESP-NOW requires STA mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed!");
    while (true);
  }

  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  // Receiver uses interrupt callback, no loop code needed
}
