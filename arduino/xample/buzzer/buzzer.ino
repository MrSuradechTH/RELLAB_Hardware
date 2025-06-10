const int buzzer = 2;
const int freq = 2700;
const int resolution = 8;
 
void setup(){
  // configure LED PWM
  ledcAttach(buzzer, freq, resolution);
}
 
void loop(){
  for(int dutyCycle = 0; dutyCycle <= 255; dutyCycle++){
    ledcWrite(buzzer, dutyCycle);
    delay(5);
  }
  for(int dutyCycle = 255; dutyCycle >= 0; dutyCycle--){
    ledcWrite(buzzer, dutyCycle);   
    delay(5);
  }
}
