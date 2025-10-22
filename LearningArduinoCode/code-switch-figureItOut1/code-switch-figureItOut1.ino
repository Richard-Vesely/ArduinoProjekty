// 🔹 DEMO 1: switch on random number
// Predict what the program will print before running!

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0)); // make it random

  int x = random(0, 4); // random number 0–3
  Serial.print("x = ");
  Serial.println(x);

  switch (x) {
    case 0:
      Serial.println("😴 Lazy mode");
      break;
    case 1:
      Serial.println("🚀 Boost mode");
      break;
    case 2:
      Serial.println("💡 Idea mode");
      break;
    case 3:
      Serial.println("🔥 Chaos mode");
      break;
    default:
      Serial.println("🌀 Huh?");
      break;
  }
}

void loop() {}
