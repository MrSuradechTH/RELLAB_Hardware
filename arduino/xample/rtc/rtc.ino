#include <Wire.h>
#include "pcf8563.h"

PCF8563_Class rtc;

void setup()
{
    Serial.begin(115200);
    Wire.begin(21, 22);
    rtc.begin();
//    rtc.setDateTime(2025, 1, 20, 10, 51, 30);
}

void loop()
{
    Serial.println(
      String(rtc.getDateTime().month) + "/" +
      String(rtc.getDateTime().day) + "/" +
      String(rtc.getDateTime().year) + " " +
      String(rtc.getDateTime().hour) + ":" +
      String(rtc.getDateTime().minute) + ":" +
      String(rtc.getDateTime().second)
    );
    delay(1000);
}
