#include <esp_now.h>
#include <WiFi.h>

char message[7];
int leftValue  = 0;
int rightValue = 0;

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len >= 6) {
    memcpy(message, data, 6);
    message[6] = '\0';
    leftValue  = atoi(String(message).substring(0, 3).c_str());
    rightValue = atoi(String(message).substring(3, 6).c_str());
    Serial.print("Left: ");
    Serial.print(leftValue);
    Serial.print("  Right: ");
    Serial.println(rightValue);
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init error!");
    return;
  }

  esp_now_register_recv_cb(onReceive);
}

void loop() {}
