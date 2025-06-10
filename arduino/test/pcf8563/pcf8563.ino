#include <Wire.h>
#include "pcf8563.h"

PCF8563_Class rtc;

void setup()
{
    Serial.begin(2000000);
    Wire.begin(21, 22);
    rtc.begin();
    rtc.setDateTime(2025, 1, 29, 23, 59, 50);
}

void loop()
{
  for (int i = 0; i <= 10; i++) {
    Serial.print(rtc_get_date_time());
    Serial.printf(" Calculated Timestamp: %lu\n", rtc_get_timestamp());
    delay(1000);
    if (i == 10) {
      rtc_update_rtc_date_time(1738195199);
      i == 0;
    }
  }
}


unsigned long rtc_date_to_timestamp(RTC_Date date) {
    struct tm t;
    t.tm_year = date.year - 1900;
    t.tm_mon = date.month - 1;
    t.tm_mday = date.day;
    t.tm_hour = date.hour;
    t.tm_min = date.minute;
    t.tm_sec = date.second;
    t.tm_isdst = -1;

    time_t timestamp = mktime(&t);
    return (unsigned long)timestamp;
}

RTC_Date rtc_timestamp_to_date(unsigned long timestamp) {
    struct tm *t;
    time_t rawtime = (time_t)timestamp;
    t = gmtime(&rawtime);

    RTC_Date date;
    date.year = t->tm_year + 1900;
    date.month = t->tm_mon + 1;
    date.day = t->tm_mday;
    date.hour = t->tm_hour;
    date.minute = t->tm_min;
    date.second = t->tm_sec;

    return date;
}

void rtc_update_rtc_date_time(unsigned long time_stamp) {
  RTC_Date new_date = rtc_timestamp_to_date(time_stamp);
  rtc.setDateTime(new_date.year, new_date.month, new_date.day, new_date.hour, new_date.minute, new_date.second);
}

String rtc_get_date_time() {
  RTC_Date date = rtc.getDateTime();
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02d/%02d/%04d %02d:%02d:%02d", date.month, date.day, date.year, date.hour, date.minute, date.second);
  return String(buffer);
}

unsigned long rtc_get_timestamp() {
  RTC_Date date = rtc.getDateTime();
  return rtc_date_to_timestamp(date);
}

void rtc_show_date_time() {
  RTC_Date date = rtc.getDateTime();
  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d\n", date.month, date.day, date.year, date.hour, date.minute, date.second);
}