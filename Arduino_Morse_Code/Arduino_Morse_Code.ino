const int LED_PIN = 13;   // vestavěná LED na Arduino Leonardo

// Časování Morseovky
const int DOT_DURATION = 200;          // tečka
const int DASH_DURATION = DOT_DURATION * 3;   // čárka
const int SYMBOL_GAP = DOT_DURATION;          // mezera mezi tečkou a čárkou v rámci jednoho písmene
const int LETTER_GAP = DOT_DURATION * 3;      // mezera mezi písmeny
const int WORD_GAP = DOT_DURATION * 7;        // mezera mezi slovy

String inputMessage = "";

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(9600);
  while (!Serial) {
    ; // počkej na připojení Serialu (Leonardo to potřebuje)
  }

  Serial.println("Napis zpravu a stiskni Enter.");
}

void loop() {
  if (Serial.available() > 0) {
    inputMessage = Serial.readStringUntil('\n');
    inputMessage.trim();

    if (inputMessage.length() > 0) {
      Serial.print("Blikam: ");
      Serial.println(inputMessage);

      blinkMessage(inputMessage);

      Serial.println("Hotovo. Napis dalsi zpravu.");
    }
  }
}

// 1) Vezme celou zprávu a pošle ji znak po znaku dál
void blinkMessage(String message) {
  for (int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);

    if (c == ' ') {
      delay(WORD_GAP);
    } else {
      String morse = charToMorse(c);

      if (morse.length() > 0) {
        blinkMorseSequence(morse);

        // mezera mezi písmeny
        delay(LETTER_GAP);
      }
    }
  }
}

// 2) Přeloží jeden znak do Morseovy abecedy
String charToMorse(char c) {
  c = toupper(c);

  switch (c) {
    case 'A': return ".-";
    case 'B': return "-...";
    case 'C': return "-.-.";
    case 'D': return "-..";
    case 'E': return ".";
    case 'F': return "..-.";
    case 'G': return "--.";
    case 'H': return "....";
    case 'I': return "..";
    case 'J': return ".---";
    case 'K': return "-.-";
    case 'L': return ".-..";
    case 'M': return "--";
    case 'N': return "-.";
    case 'O': return "---";
    case 'P': return ".--.";
    case 'Q': return "--.-";
    case 'R': return ".-.";
    case 'S': return "...";
    case 'T': return "-";
    case 'U': return "..-";
    case 'V': return "...-";
    case 'W': return ".--";
    case 'X': return "-..-";
    case 'Y': return "-.--";
    case 'Z': return "--..";

    case '0': return "-----";
    case '1': return ".----";
    case '2': return "..---";
    case '3': return "...--";
    case '4': return "....-";
    case '5': return ".....";
    case '6': return "-....";
    case '7': return "--...";
    case '8': return "---..";
    case '9': return "----.";

    default: return "";
  }
}

// 3) Vezme řetězec typu ".-.." a vybliká ho
void blinkMorseSequence(String morse) {
  for (int i = 0; i < morse.length(); i++) {
    char symbol = morse.charAt(i);

    if (symbol == '.') {
      blinkDot();
    } else if (symbol == '-') {
      blinkDash();
    }

    // mezera mezi symboly jednoho písmene
    if (i < morse.length() - 1) {
      delay(SYMBOL_GAP);
    }
  }
}

// 4) Pomocné funkce pro tečku a čárku
void blinkDot() {
  digitalWrite(LED_PIN, HIGH);
  delay(DOT_DURATION);
  digitalWrite(LED_PIN, LOW);
}

void blinkDash() {
  digitalWrite(LED_PIN, HIGH);
  delay(DASH_DURATION);
  digitalWrite(LED_PIN, LOW);
}