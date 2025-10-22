int n = 10;       // až do kterého čísla se bude sčítat
int total = 0;    // proměnná pro součet

void setup() {
  Serial.begin(9600);

  for (int i = 1; i <= n; i++) {
    total = total + i;
  }

  Serial.print("Součet 1–");
  Serial.print(n);
  Serial.print(" je: ");
  Serial.println(total);
}

void loop() {}
