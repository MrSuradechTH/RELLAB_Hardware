#include <Adafruit_MAX31855.h>

int DO = 19;
int CS = 5;
int CLK = 18;

Adafruit_MAX31855 thermocouple(CLK, CS, DO);

void setup() {
  Serial.begin(9600);
  
  while (!Serial) delay(1);
  
  Serial.println("MAX31855 test");
  delay(500);
}

void loop() {
   double c = thermocouple.readCelsius();
  if (isnan(c)) {
    Serial.println("Something wrong with thermocouple!");
  }else {
    Serial.print("Internal Temp = ");
    Serial.print(thermocouple.readInternal());
    Serial.print(" C = "); 
    Serial.println(c);
  }
  delay(100);
}
