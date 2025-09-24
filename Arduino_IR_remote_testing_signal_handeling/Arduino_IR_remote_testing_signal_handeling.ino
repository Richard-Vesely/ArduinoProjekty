#include <IRremote.h>

#define IR_PIN 2

// --- KONSTANTY PRO TVÁ TLAČÍTKA ---
const unsigned long ONE      = 0xBA45FF00;
const unsigned long TWO      = 0xB946FF00;
const unsigned long THREE    = 0xB847FF00;
const unsigned long FOUR     = 0xBB44FF00;
const unsigned long FIVE     = 0xBF40FF00;
const unsigned long SIX      = 0xBC43FF00;
const unsigned long SEVEN    = 0xF807FF00;
const unsigned long EIGHT    = 0xEA15FF00;
const unsigned long NINE     = 0xF609FF00;
const unsigned long STARBTN  = 0xE916FF00;
const unsigned long ZERO     = 0xE619FF00;
const unsigned long HASHBTN  = 0xF20DFF00;
const unsigned long UPBTN    = 0xE718FF00;
const unsigned long LEFTBTN  = 0xF708FF00;
const unsigned long OKBTN    = 0xE31CFF00;
const unsigned long RIGHTBTN = 0xA55AFF00;
const unsigned long DOWNBTN  = 0xAD52FF00;

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println("IR prijimac pripraven. Zmackni tlacitko na ovladaci.");
}

void loop() {
  if (!IrReceiver.decode()) return;

  unsigned long signal = IrReceiver.decodedIRData.decodedRawData;

  // --- LADICI VYSTUP ---
  
  Serial.print("RAW=0x");
  Serial.print(signal, HEX);
  Serial.print("  =>  ");

  switch (signal) {
    case ONE:      Serial.println("ONE"); break;
    case TWO:      Serial.println("TWO"); break;
    case THREE:    Serial.println("THREE"); break;
    case FOUR:     Serial.println("FOUR"); break;
    case FIVE:     Serial.println("FIVE"); break;
    case SIX:      Serial.println("SIX"); break;
    case SEVEN:    Serial.println("SEVEN"); break;
    case EIGHT:    Serial.println("EIGHT"); break;
    case NINE:     Serial.println("NINE"); break;
    case ZERO:     Serial.println("ZERO"); break;
    case STARBTN:  Serial.println("* (STAR)"); break;
    case HASHBTN:  Serial.println("# (HASH)"); break;
    case UPBTN:    Serial.println("UP"); break;
    case DOWNBTN:  Serial.println("DOWN"); break;
    case LEFTBTN:  Serial.println("LEFT"); break;
    case RIGHTBTN: Serial.println("RIGHT"); break;
    case OKBTN:    Serial.println("OK"); break;
    case 0x0:      Serial.println("REPEAT (držení tlacitka)"); break;
    default:       Serial.println("Neznamy kod"); break;
  }

  IrReceiver.resume();
}