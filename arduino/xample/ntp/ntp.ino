#include <WiFi.h>

const char* ssid = "rellab";
const char* password = "1234567890";

const char* daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#include <stdio.h>

void setup()
{
  Serial.begin(2000000);
  
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  configTime(0, 0, "192.168.0.100");

  delay(1000);
}

void loop()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  
  Serial.print("Current time: ");
  Serial.print(timeinfo.tm_hour + 7);
  Serial.print(':');
  Serial.print(timeinfo.tm_min);
  Serial.print(':');
  Serial.print(timeinfo.tm_sec);
  Serial.print(" on ");
  Serial.print(timeinfo.tm_mday);
  Serial.print('/');
  Serial.print(timeinfo.tm_mon + 1);
  Serial.print('/');
  Serial.println(timeinfo.tm_year + 1900);
  Serial.print(" day of week ");
  Serial.println(daysOfTheWeek[timeinfo.tm_wday]);
  delay (1000);
}
