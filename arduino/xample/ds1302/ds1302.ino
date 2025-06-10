#include <stdio.h>
#include <DS1302.h>

DS1302 rtc(14, 12, 13);

void setup() {
  Serial.begin(2000000);
  rtc.writeProtect(false);
  rtc.halt(false);
  Time t(2024, 1, 26, 21, 42, 50, Time::kFriday);
  rtc.time(t);
}

void loop() {
  Time t = rtc.time();
  Serial.print("Current time: ");
  Serial.print(t.hr);
  Serial.print(':');
  Serial.print(t.min);
  Serial.print(':');
  Serial.print(t.sec);
  Serial.print(" on ");
  Serial.print(t.date);
  Serial.print('/');
  Serial.print(t.mon);
  Serial.print('/');
  Serial.print(t.yr);
  Serial.print(" day ");
  Serial.println(t.day);

  delay(1000);
}
