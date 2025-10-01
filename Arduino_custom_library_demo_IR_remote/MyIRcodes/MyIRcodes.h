/************************************************************
ZAPOJENÍ (pro testovací příklad s IRremote v3+):
- IR přijímač:
    VCC → 5V
    GND → GND
    OUT → D2 (Arduino Leonardo)
- Tento soubor je pouze "knihovna" s konstantami a pomocnými funkcemi.

Úkoly, pro pochopení kódu:
1) Jak ti pomáhá mít všechny IR kódy a názvy tlačítek v jedné knihovně?
2) Co vrátí decode(), když přijde opakovací kód 0x0 (NEC repeat)? Proč potřebuje "last"?
3) Jak bys přidal nové tlačítko? (doplnit konstantu, enum, case v decode(), label v label())

************************************************************/

#ifndef MY_IRCODES_H
#define MY_IRCODES_H

#include <Arduino.h>

namespace MyIR {

  // --- Pojmenované konstanty pro tvoje kódy ---
  constexpr uint32_t ONE      = 0xBA45FF00;
  constexpr uint32_t TWO      = 0xB946FF00;
  constexpr uint32_t THREE    = 0xB847FF00;
  constexpr uint32_t FOUR     = 0xBB44FF00;
  constexpr uint32_t FIVE     = 0xBF40FF00;
  constexpr uint32_t SIX      = 0xBC43FF00;
  constexpr uint32_t SEVEN    = 0xF807FF00;
  constexpr uint32_t EIGHT    = 0xEA15FF00;
  constexpr uint32_t NINE     = 0xF609FF00;
  constexpr uint32_t STARBTN  = 0xE916FF00;
  constexpr uint32_t ZERO     = 0xE619FF00;
  constexpr uint32_t HASHBTN  = 0xF20DFF00;
  constexpr uint32_t UPBTN    = 0xE718FF00;
  constexpr uint32_t LEFTBTN  = 0xF708FF00;
  constexpr uint32_t OKBTN    = 0xE31CFF00;
  constexpr uint32_t RIGHTBTN = 0xA55AFF00;
  constexpr uint32_t DOWNBTN  = 0xAD52FF00;

  constexpr uint32_t NEC_REPEAT = 0x0;

  // Logická jména tlačítek
  enum Button : uint8_t {
    BTN_NONE = 0,
    BTN_1, BTN_2, BTN_3,
    BTN_4, BTN_5, BTN_6,
    BTN_7, BTN_8, BTN_9,
    BTN_STAR, BTN_0, BTN_HASH,
    BTN_UP, BTN_LEFT, BTN_OK, BTN_RIGHT, BTN_DOWN
  };

  // Překlad číselného kódu na Button s ošetřením NEC repeat (0x0).
  // Parametr 'last' drží poslední skutečný stisk (předávej proměnnou z hlavního kódu).
  inline Button decode(uint32_t rawCode, Button& last) {
    if (rawCode == NEC_REPEAT) {
      return last; // držení tlačítka
    }
    Button found = BTN_NONE;
    switch (rawCode) {
      case ONE:      found = BTN_1; break;
      case TWO:      found = BTN_2; break;
      case THREE:    found = BTN_3; break;
      case FOUR:     found = BTN_4; break;
      case FIVE:     found = BTN_5; break;
      case SIX:      found = BTN_6; break;
      case SEVEN:    found = BTN_7; break;
      case EIGHT:    found = BTN_8; break;
      case NINE:     found = BTN_9; break;
      case STARBTN:  found = BTN_STAR; break;
      case ZERO:     found = BTN_0; break;
      case HASHBTN:  found = BTN_HASH; break;
      case UPBTN:    found = BTN_UP; break;
      case LEFTBTN:  found = BTN_LEFT; break;
      case OKBTN:    found = BTN_OK; break;
      case RIGHTBTN: found = BTN_RIGHT; break;
      case DOWNBTN:  found = BTN_DOWN; break;
      default:       found = BTN_NONE; break;
    }
    if (found != BTN_NONE) last = found;
    return found;
  }

  // Hezký popisek tlačítka
  inline const char* label(Button b) {
    switch (b) {
      case BTN_1: return "1";
      case BTN_2: return "2";
      case BTN_3: return "3";
      case BTN_4: return "4";
      case BTN_5: return "5";
      case BTN_6: return "6";
      case BTN_7: return "7";
      case BTN_8: return "8";
      case BTN_9: return "9";
      case BTN_0: return "0";
      case BTN_STAR: return "*";
      case BTN_HASH: return "#";
      case BTN_UP: return "UP";
      case BTN_DOWN: return "DOWN";
      case BTN_LEFT: return "LEFT";
      case BTN_RIGHT: return "RIGHT";
      case BTN_OK: return "OK";
      default: return "(none)";
    }
  }

  // Jednoduchá pomůcka: je to některé z námi známých tlačítek?
  inline bool isKnown(Button b) { return b != BTN_NONE; }

} // namespace MyIR

#endif // MY_IRCODES_H
