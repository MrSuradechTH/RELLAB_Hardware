const int buzzer = 2;
const int freq = 2700;
const int resolution = 8;

void setup() {
  Serial.begin(2000000);
  ledcSetup(0, freq, resolution); //2.7kHz, 8-bit resolution 0-255
  ledcAttachPin(buzzer, 0);
}

void loop() {
  ledcWrite(0, 150);
  delay(2000);
  ledcWrite(0, 0);
  delay(2000);
}