/*
ZAPOJENÍ (CZ):
IR: GND->GND, VCC->5V, OUT->D2 (nebo D3 a přepsat RECV_PIN)

ÚKOLY:
1) Co vypíše při příjmu signálu?
2) Proč je IrReceiver.resume() nutné?
*/

#include <IRremote.h>
const uint8_t RECV_PIN = 2;

void setup(){
  Serial.begin(115200);
  unsigned long t0 = millis(); while(!Serial && millis()-t0<5000){}
  IrReceiver.begin(RECV_PIN, DISABLE_LED_FEEDBACK); // LED feedback vypnuto (někdy ruší)
  Serial.println(F("IRremote v3+: cekam na signal..."));
}
void loop(){
  if(IrReceiver.decode()){
    Serial.print(F("Proto=")); Serial.print(IrReceiver.decodedIRData.protocol);
    Serial.print(F("  Code=0x")); Serial.println(IrReceiver.decodedIRData.decodedRawData, HEX);
    IrReceiver.resume();
  }
}
