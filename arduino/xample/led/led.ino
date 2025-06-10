int led[3] = {17, 16, 15};

void setup () {
  pinMode(led[0], OUTPUT);
  pinMode(led[1], OUTPUT);
  pinMode(led[2], OUTPUT);
}

void loop() {
  digitalWrite(led[0], HIGH);
  delay(500);
  digitalWrite(led[0], LOW);
  delay(500);
  digitalWrite(led[1], HIGH);
  delay(500);
  digitalWrite(led[1], LOW);
  delay(500);
  digitalWrite(led[2], HIGH);
  delay(500);
  digitalWrite(led[2], LOW);
  delay(500);
}
