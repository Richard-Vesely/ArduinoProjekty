/************************************************************
ZAPOJENÍ (Arduino Leonardo + IR přijímač):
- IR přijímač:
   VCC → 5V
   GND → GND
   OUT → D2  (digitální pin 2)
- LED pásek zatím nepřipojuj – tohle je „čtečka“ ovladače.

Úkoly, pro pochopení kódu:
1) Co vrací funkce findButtonByCode(), když přijde kód 0x0? Proč?
2) Jak bys přidal nové tlačítko? Které tři části kódu je třeba upravit?
3) Kde v kódu jednoduše navážeš konkrétní akci (např. změna barvy LED) na stisk tlačítka „▲“?

************************************************************/

#include <IRremote.h>

#define IR_PIN 2

// 1) JASNÁ ENUMERACE TLAČÍTEK (logické názvy, ne hex)
enum Button {
  BTN_NONE = 0,
  BTN_1, BTN_2, BTN_3,
  BTN_4, BTN_5, BTN_6,
  BTN_7, BTN_8, BTN_9,
  BTN_STAR, BTN_0, BTN_HASH,
  BTN_UP, BTN_LEFT, BTN_OK, BTN_RIGHT, BTN_DOWN
};

// 2) STRUKTURA MAPOVÁNÍ: přijatý 32bit kód → Button + „hezké“ jméno
struct ButtonMap {
  uint32_t code;
  Button   btn;
  const char* label;
};

// Tvoje kódy (Protocol=8), seřazeno podle tebou zjištěného pořadí:
static const ButtonMap BUTTONS[] PROGMEM = {
  {0xBA45FF00, BTN_1,    "1"},
  {0xB946FF00, BTN_2,    "2"},
  {0xB847FF00, BTN_3,    "3"},
  {0xBB44FF00, BTN_4,    "4"},
  {0xBF40FF00, BTN_5,    "5"},
  {0xBC43FF00, BTN_6,    "6"},
  {0xF807FF00, BTN_7,    "7"},
  {0xEA15FF00, BTN_8,    "8"},
  {0xF609FF00, BTN_9,    "9"},
  {0xE916FF00, BTN_STAR, "*"},
  {0xE619FF00, BTN_0,    "0"},
  {0xF20DFF00, BTN_HASH, "#"},
  {0xE718FF00, BTN_UP,   "▲"},
  {0xF708FF00, BTN_LEFT, "◄"},
  {0xE31CFF00, BTN_OK,   "OK"},
  {0xA55AFF00, BTN_RIGHT,"►"},
  {0xAD52FF00, BTN_DOWN, "▼"}
};

Button lastNonRepeat = BTN_NONE;  // pro zpracování NEC repeat (0x0)

// 3) FUNKCE NAJDE TLAČÍTKO DLE KÓDU; 0x0 = REPEAT → vrátí poslední reálné tlačítko
Button findButtonByCode(uint32_t code) {
  if (code == 0x0) {
    return lastNonRepeat; // NEC repeat
  }
  for (size_t i = 0; i < sizeof(BUTTONS)/sizeof(BUTTONS[0]); i++) {
    ButtonMap m;
    memcpy_P(&m, &BUTTONS[i], sizeof(ButtonMap));
    if (m.code == code) {
      lastNonRepeat = m.btn; // aktualizuj poslední reálný stisk
      return m.btn;
    }
  }
  return BTN_NONE;
}

// 4) Volitelně: převod Button → textový název (pro Serial log)
const char* buttonLabel(Button b) {
  for (size_t i = 0; i < sizeof(BUTTONS)/sizeof(BUTTONS[0]); i++) {
    ButtonMap m;
    memcpy_P(&m, &BUTTONS[i], sizeof(ButtonMap));
    if (m.btn == b) return m.label;
  }
  return "(neznámé)";
}

void setup() {
  Serial.begin(115200);
  IrReceiver.begin(IR_PIN, ENABLE_LED_FEEDBACK);
  Serial.println(F("IR prijimac pripraven. Zmackni tlacitko na ovladaci."));
}

void loop() {
  if (!IrReceiver.decode()) return;

  uint32_t raw = IrReceiver.decodedIRData.decodedRawData; // u NEC 32bit vzor
  Button b = findButtonByCode(raw);

  Serial.print(F("RAW=0x"));
  Serial.print(raw, HEX);
  Serial.print(F("  →  "));

  if (b == BTN_NONE) {
    Serial.println(F("Nenamapovano (nebo zadny predchozi pro repeat)."));
  } else {
    Serial.print(F("TLAČÍTKO: "));
    Serial.println(buttonLabel(b));
  }

  // === TADY V BUDOUCNU DĚLEJ AKCE (LED efekty atd.) ===
  // příklad rozcestníku:
  switch (b) {
    case BTN_OK:
      // TODO: přepni režim animace
      break;
    case BTN_UP:
      // TODO: zvyšit jas
      break;
    case BTN_DOWN:
      // TODO: snížit jas
      break;
    case BTN_LEFT:
      // TODO: předchozí barva
      break;
    case BTN_RIGHT:
      // TODO: další barva
      break;
    default:
      break;
  }

  IrReceiver.resume();
}
