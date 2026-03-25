/*
 * Arduino Leonardo — 9g Servo Range Testing
 *
 * Purpose: Periodically sweeps a 9g servo (e.g. SG90 / MG90S) from its
 *          minimum to maximum position and back, printing the current
 *          angle to Serial so you can observe the actual reachable range.
 *
 * ========== WIRING ==========
 *
 *   9g Servo        Arduino Leonardo
 *   ─────────       ────────────────
 *   Red wire   -->  5V   (servo needs 4.8-6V)
 *   Brown wire -->  GND
 *   Orange wire ->  Pin 9  (signal / PWM)
 *
 *   NOTE: If the servo jitters or the board resets, the USB port may not
 *         supply enough current. In that case, power the servo from an
 *         external 5V supply (connect GND of the supply to board GND).
 *
 * ========== BOARD SETUP IN ARDUINO IDE ==========
 *
 *   1. Select board: "Arduino Leonardo".
 *   2. No extra libraries needed — uses the built-in Servo library.
 *   3. Upload this sketch and open Serial Monitor at 115200 baud.
 *   4. Wait for the serial port to connect (Leonardo uses native USB).
 *
 * =================================================
 */

#include <Servo.h>

// ============ TUNING PARAMETERS ============
const int SERVO_PIN       = 9;     // PWM pin for servo signal
const int PULSE_MIN_US    = 400;   // minimum pulse width in microseconds (try lowering to push further)
const int PULSE_MAX_US    = 2600;  // maximum pulse width in microseconds (try raising to push further)
const int STEP_US         = 50;    // microseconds per step (bigger = faster sweep)
const int STEP_DELAY_MS   = 5;     // milliseconds between steps (smaller = faster sweep)
const int PAUSE_AT_END_MS = 1000;  // pause at each extreme (ms)
// ===========================================

Servo myServo;

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for native USB serial port to connect (Leonardo-specific)
  }
  delay(500);
  Serial.println();
  Serial.println("=== 9g Servo Range Test (microseconds) ===");
  Serial.print("Sweeping from ");
  Serial.print(PULSE_MIN_US);
  Serial.print("us to ");
  Serial.print(PULSE_MAX_US);
  Serial.println("us and back.");
  Serial.println("Standard range is 544-2400us. Going beyond may push past 180 degrees.");
  Serial.println();

  myServo.attach(SERVO_PIN, PULSE_MIN_US, PULSE_MAX_US);
}

void loop() {
  // Sweep from min to max pulse (clockwise)
  Serial.println(">> Sweeping CW: min -> max");
  for (int us = PULSE_MIN_US; us <= PULSE_MAX_US; us += STEP_US) {
    myServo.writeMicroseconds(us);
    Serial.print("Pulse: ");
    Serial.print(us);
    Serial.println("us");
    delay(STEP_DELAY_MS);
  }

  Serial.println("-- Pausing at max --");
  delay(PAUSE_AT_END_MS);

  // Sweep from max to min pulse (counter-clockwise)
  Serial.println(">> Sweeping CCW: max -> min");
  for (int us = PULSE_MAX_US; us >= PULSE_MIN_US; us -= STEP_US) {
    myServo.writeMicroseconds(us);
    Serial.print("Pulse: ");
    Serial.print(us);
    Serial.println("us");
    delay(STEP_DELAY_MS);
  }

  Serial.println("-- Pausing at min --");
  delay(PAUSE_AT_END_MS);

  Serial.println("=== Cycle complete ===");
  Serial.println();
}
