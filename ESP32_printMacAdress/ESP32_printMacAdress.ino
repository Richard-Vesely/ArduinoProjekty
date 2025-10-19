/************************************************************
🔌 ZAPOJENÍ (pro děti)
- Není potřeba nic připojovat – stačí připojit ESP32 k počítači přes USB.
- Otevři v Arduino IDE: Nástroje → Sériový monitor → 115200 baud.

🧩 ÚKOLY PRO POCHOPENÍ KÓDU:
1️⃣ Proč musíme zavolat WiFi.mode(WIFI_STA), než čteme MAC adresu?
2️⃣ Co by se stalo, kdybych přepnul na WIFI_AP (access point)?
3️⃣ Kde se v programu rozhoduje, jak často se MAC vypisuje?
4️⃣ Změň čas tak, aby se vypisovala každou sekundu.
************************************************************/

#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(500);

  // Aktivujeme WiFi rozhraní, jinak vrátí 00:00:00:00:00:00
  WiFi.mode(WIFI_STA);

  Serial.println("ESP32 MAC Address Printer");
  Serial.println("---------------------------");
}

void loop() {
  String mac = WiFi.macAddress();
  Serial.print("MAC Address: ");
  Serial.println(mac);
  delay(2000);
}
