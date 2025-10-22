// Traffic light (millis-based) with manual/auto and double-press mode toggle
// Wiring:
//   RED -> pin 2 (through 220Ω to GND)
//   YEL -> pin 3 (through 220Ω to GND)
//   GRN -> pin 4 (through 220Ω to GND)
//   BTN -> pin 7 to GND (uses INPUT_PULLUP)

const int RED = 2, YEL = 3, GRN = 4;
const int BTN = 7;

// ---- timings (ms) for each stage in AUTO mode ----
enum Stage { S_RED = 0, S_RED_YEL, S_GRN, S_YEL, STAGE_COUNT };
const unsigned long STAGE_MS[STAGE_COUNT] = {
  3000, // RED
  1000, // RED+YEL
  3000, // GREEN
  1000  // YELLOW
};

// ---- button + click detection ----
const unsigned long DEBOUNCE_MS   = 30;
const unsigned long DOUBLE_MS     = 400;  // max gap between presses for double-click

// button state tracking (for debouncing)
int  rawBtnPrev        = HIGH;   // last raw read
int  stableBtnState    = HIGH;   // debounced state (HIGH = not pressed due to pullup)
int  stableBtnPrev     = HIGH;
unsigned long lastBounceAt = 0;

// click detection
unsigned long firstClickAt = 0;
uint8_t clickCount = 0;

// ---- mode + stage state ----
bool autoMode = true;                 // true = auto cycle by time, false = manual
Stage stage   = S_RED;
unsigned long stageStartedAt = 0;

void showStage(Stage s) {
  switch (s) {
    case S_RED:
      digitalWrite(RED, HIGH); digitalWrite(YEL, LOW);  digitalWrite(GRN, LOW);
      break;
    case S_RED_YEL:
      digitalWrite(RED, HIGH); digitalWrite(YEL, HIGH); digitalWrite(GRN, LOW);
      break;
    case S_GRN:
      digitalWrite(RED, LOW);  digitalWrite(YEL, LOW);  digitalWrite(GRN, HIGH);
      break;
    case S_YEL:
      digitalWrite(RED, LOW);  digitalWrite(YEL, HIGH); digitalWrite(GRN, LOW);
      break;
  }
}

void nextStage() {
  stage = static_cast<Stage>((stage + 1) % STAGE_COUNT);
  stageStartedAt = millis();
  showStage(stage);
}

void toggleMode() {
  autoMode = !autoMode;
  // reset stage timer so auto resumes cleanly
  stageStartedAt = millis();
  // Optional: brief visual confirmation of mode switch (non-blocking idea):
  // You can e.g. flash all LEDs quickly here using a small state, but keeping it simple.
}

void setup() {
  pinMode(RED, OUTPUT);
  pinMode(YEL, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);

  showStage(stage);
  stageStartedAt = millis();

  Serial.begin(115200);
  Serial.println(F("Millis Traffic Light - single press = manual advance, double press = toggle auto/manual"));
}

void loop() {
  unsigned long now = millis();

  // -------- Debounce button (no delay) --------
  int raw = digitalRead(BTN);
  if (raw != rawBtnPrev) {
    lastBounceAt = now;
    rawBtnPrev = raw;
  }
  if (now - lastBounceAt > DEBOUNCE_MS) {
    // debounced state
    stableBtnState = raw;
  }

  // detect press event (HIGH -> LOW because of INPUT_PULLUP)
  bool pressEvent = (stableBtnPrev == HIGH && stableBtnState == LOW);
  stableBtnPrev = stableBtnState;

  // -------- Click / double-click handling --------
  // We immediately handle a double-press; single press is only acted on
  // after the double-click window expires.
  if (pressEvent) {
    if (clickCount == 0) {
      clickCount = 1;
      firstClickAt = now;
    } else if (clickCount == 1 && (now - firstClickAt) <= DOUBLE_MS) {
      // double press detected
      clickCount = 0;
      toggleMode();
      Serial.print(F("Mode: "));
      Serial.println(autoMode ? F("AUTO") : F("MANUAL"));
    } else {
      // too slow; treat as new first click
      clickCount = 1;
      firstClickAt = now;
    }
  }

  // If we had only one click and the double window expired,
  // treat it as a single click action (manual advance).
  if (clickCount == 1 && (now - firstClickAt) > DOUBLE_MS) {
    // single press action
    if (!autoMode) {
      nextStage();
      Serial.print(F("Manual advance to stage: "));
      Serial.println(stage);
    }
    clickCount = 0;
  }

  // -------- AUTO mode timing --------
  if (autoMode) {
    if (now - stageStartedAt >= STAGE_MS[stage]) {
      nextStage();
    }
  }

  // Nothing blocks; loop runs fast and responsively.
}
