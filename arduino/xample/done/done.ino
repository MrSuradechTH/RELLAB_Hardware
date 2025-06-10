//extension
#include <Arduino.h>
#include <stdio.h>
#include <TimeLib.h>

//wifi
#include <WiFi.h>

//const char* ssid = "rellab";
const char* ssid = "Unknown";
const char* password = "1234567890";

//websocket
#include <WebSocketsClient.h>

WebSocketsClient webSocket;

bool webSocketConnection = false;

//oled
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 16
Adafruit_SSD1306 display(OLED_RESET);
char str[255];

//max31855
#include <Adafruit_MAX31855.h>
double C;

int DO = 19;
int CS = 5;
int CLK = 18;

Adafruit_MAX31855 Max31855(18, 5, 19); //CLK, CS, DO

//ds1302
#include <DS1302.h>

DS1302 rtc(14, 12, 13); //RST, DAT, CLK

bool rtc_first_time = true;
int sec,ms_old,ms_now = 0;

//ntp
const char* ntp_server = "192.168.100.100";

const char* daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
const Time::Day TimeDay_daysOfTheWeek[7] = {Time::kSunday, Time::kMonday, Time::kTuesday, Time::kWednesday, Time::kThursday, Time::kFriday, Time::kSaturday};

//etc
int LED = 2;
int display_count = 50;

//cal
int SW = 27;
int i = 0;
long millis_old,millis_now;
bool start = false;
int gen_start = -200,gen_stop = 1000;
float gen_step = 0.1;
int gen_range = 12000;
float map_max[6000],map_gen[6000];
int step_time = 1000;

void manual_cal() {
  if (digitalRead(SW) == HIGH) {
    digitalWrite(LED, LOW);
  }else {
    start = true;
    Serial.println("Start cal");
    Serial.println("Gen start : " + String(gen_start));
    Serial.println("Gen stop : " + String(gen_stop));
    Serial.println("Gen step : " + String(gen_step));
    Serial.println("Gen range : " + String(gen_range));
    digitalWrite(LED, HIGH);
  }
  millis_now = millis();
  if (millis_now - millis_old >= step_time && i < gen_range && start == true) {
    map_max[i] = Max31855.readCelsius();
    map_gen[i] = gen_start + (gen_step * i);
    Serial.println("MAX[" + String(i) + "] = " + String(map_max[i]) + " C : GEN[" + String(i) + "] = " + String(map_gen[i]) + " C");
    i++;
    millis_old = millis_now;
  }else if (i > gen_range && start == true){
    start = false;
    i = 0;
    
    Serial.print("MAX_CAL[");
    for(int i = 0;i < gen_range;i++) {
      if (i + 1 >= gen_range) {
        Serial.print(String(map_max[i]));
        Serial.println("]");
      }else {
        Serial.print(String(map_max[i]) + ",");
      }
    }

    Serial.print("GEN_CAL[");
    for(int i = 0;i < gen_range;i++) {
      if (i + 1 >= gen_range) {
        Serial.print(String(map_gen[i]));
        Serial.println("]");
      }else {
        Serial.print(String(map_gen[i]) + ",");
      }
    }
  }
  digitalWrite(LED, HIGH);
  delay(100);
  digitalWrite(LED, LOW);
  delay(100);
}

//websocket
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_DISCONNECTED:
        webSocketConnection = false;
        Serial.println("Websocket is disconnected");
        break;
      case WStype_CONNECTED:
        webSocketConnection = true;
        Serial.println("Websocket is connected");
        break;
      case WStype_TEXT:
        Serial.printf("Websocket get text : %s\n", payload);
//        webSocket.sendTXT("Message received");
        break;
      case WStype_BIN:
        Serial.printf("Websocket get binary length : %u\n", length);
        break;
      default:
          break;
    }
}

Time unixtime2time(time_t unixtime) {
  struct tm * timeinfo;
  timeinfo = gmtime(&unixtime);

  Time t(0, 0, 0, 0, 0, 0, Time::kSunday);
  t.yr = timeinfo->tm_year + 1900;
  t.mon = timeinfo->tm_mon + 1;
  t.date = timeinfo->tm_mday;
  t.hr = timeinfo->tm_hour + 7;
  t.min = timeinfo->tm_min;
  t.sec = timeinfo->tm_sec;
  t.day = TimeDay_daysOfTheWeek[timeinfo->tm_wday];

  return t;
}

int mk_gmtime(int year, int month, int day, int hour, int minute, int second) {
  struct tm t;
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour - 7;
  t.tm_min = minute;
  t.tm_sec = second;
  t.tm_isdst = -1;
  return mktime(&t);
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(SW, INPUT_PULLUP);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  Serial.begin(2000000);
  while (!Serial) delay(1);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  webSocket.begin("192.168.100.100", 8080, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(10);

  configTime(0, 0, ntp_server);
//  manual_cal();
}

void loop() {
  webSocket.loop();
  
  if (display_count >= 50) {
    display.clearDisplay();
    display.setCursor(0,0);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Wifi : Fail");
      display.println("Wifi : Fail");
    }else {
      Serial.println("Wifi : Connected");
      display.println("Wifi : Connected");
    }
    display.setCursor(0,8);

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("NTP : Fail");
      display.println("NTP : Fail");
    }else {
      struct tm timeinfo;
      if(!getLocalTime(&timeinfo, 10)){
        Serial.println("NTP : Fail");
        display.println("NTP : Fail");
      }else {
        Serial.println("NTP : Connected");
        display.println("NTP : Connected");
    
        if (rtc_first_time == true) {
          rtc.writeProtect(false);
          rtc.halt(false);

          time_t unixTime = mktime(&timeinfo);
          Serial.print("Unix time: ");
          Serial.println(unixTime);

//          unixTime += (7 * 60 * 60);

          Time t = unixtime2time(unixTime);

//          Time t(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday + (int((timeinfo.tm_hour + 7) / 24) * 1), (timeinfo.tm_hour + 7) - (int((timeinfo.tm_hour + 7) / 24) * 24), timeinfo.tm_min, timeinfo.tm_sec, TimeDay_daysOfTheWeek[timeinfo.tm_wday + 1]);
          rtc.time(t);
      
          Serial.println("Time::kSaturday = " + String(Time::kSaturday));
          Serial.println("Timeinfo.tm_wday = " + String(TimeDay_daysOfTheWeek[timeinfo.tm_wday + 1]));
          Serial.println("Timeinfo.tm_wday = " + String(timeinfo.tm_wday));
        
//          Serial.print("Current time: ");
//          Serial.print(timeinfo.tm_hour + 7);
//          Serial.print(':');
//          Serial.print(timeinfo.tm_min);
//          Serial.print(':');
//          Serial.print(timeinfo.tm_sec);
//          Serial.print(" on ");
//          Serial.print(timeinfo.tm_mday);
//          Serial.print('/');
//          Serial.print(timeinfo.tm_mon + 1);
//          Serial.print('/');
//          Serial.print(timeinfo.tm_year + 1900);
//          Serial.print(" day of week ");
//          Serial.println(daysOfTheWeek[timeinfo.tm_wday]);
    
          rtc_first_time = false;
        }
      }
    }
    display.setCursor(0,16);
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WS : Fail");
      display.println("WS : Fail");
    }else {
      if (webSocketConnection == false) {
        Serial.println("WS : Fail");
        display.println("WS : Fail");
      }else {
        Serial.println("WS : Connected");
        display.println("WS : Connected");
      }
    }

    if (display_count >= 60) {
      display_count = 0;
    }

    display_count++;
  }else {
    Time t = rtc.time();
    C = Max31855.readCelsius();

    ms_now = millis();
    if (ms_now > ms_old + 100) {
//      if (sec != int(t.sec)) {
        int unixTime = mk_gmtime(t.yr, t.mon, t.date, t.hr, t.min, t.sec);
        webSocket.sendTXT("{\"machinename\" : \"TEST\",\"temp\" : \"" + String(C) + "\",\"second\" : \"" + String(unixTime) + "\"}");
        sec = int(t.sec);
//      }
    }
  
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
    Serial.println(t.day - 1);
    Serial.print(" day of week ");
    Serial.println(daysOfTheWeek[t.day - 1]);
    
    display.clearDisplay();
    display.setCursor(0,0);
    if (WiFi.status() != WL_CONNECTED) {
      display.println("IP : XXX.XXX.XXX.XXX");
    }else {
      display.println("IP : " + WiFi.localIP().toString());
    }
    display.setCursor(0,8);
    sprintf(str, "%02d/%02d/%0d", t.date, t.mon, t.yr);
    display.println("Date : " + String(str));
    display.setCursor(0,16);
    sprintf(str, "%02d:%02d:%02d", t.hr, t.min, t.sec);
    display.println("Time : " + String(str));
    display.setCursor(0,24);
    if (isnan(C)) {
      display.println("Temp : Error");
    }else {
      sprintf(str, "%4.2f", C);
      display.println("Temp : " + String(str) + " C");
    }

    display_count++;
  }
  display.display();
  
  delay(10);
  digitalWrite(LED, HIGH);
  delay(10);
  digitalWrite(LED, LOW);
}
