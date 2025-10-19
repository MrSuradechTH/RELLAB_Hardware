#include <SPI.h> //for oled, sd_card
#include <Wire.h> //for oled, mcp9600, pcf8563

//wifi
#include <WiFi.h>

const char* ssid = "MrSuradechTH";
const char* password = "1234567890";
int reconnect_wifi_count = 0;
bool avialable_wifi = false;

//http
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

HTTPClient http;

const char* url = "https://rellabapi.suradech.com/record/add";
const int http_timeout = 800;

//json
#include <ArduinoJson.h>

//ota
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* host_name = "DEMO_Machine";

//OLED
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 16
//Adafruit_SSD1306 display(OLED_RESET);
Adafruit_SSD1306 display(-1);

//MCP9600
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"

const uint8_t mcp9600_address[] = {0x67, 0x60, 0x65, 0x66, 0x64};
float mcp9600_max_limit[] = {180.00, 180.00, 180.00, 180.00, 180.00};
float mcp9600_min_limit[] = { -150.00, -150.00, -150.00, -150.00, -150.00};
Adafruit_MCP9600 mcp[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])];
float mcp9600_value[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])];
String mcp9600_data_name[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])] = {"CH1", "CH2", "CH3", "CH4", "CH5"};
float mcp9600_alarm[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])];

/* Set and print ambient resolution */
Ambient_Resolution ambientRes = RES_ZERO_POINT_03125;

//trimmer
const int trimmer[] = {32, 35, 34, 39, 36};
const float max_trimmer_value = 4085;
const float min_trimmer_value = 10;
const float out_max_trimmer_value = 3.00;
const float out_min_trimmer_value = -3.00;
const int trimmer_averaging = 25;
static float trimmer_value[sizeof(trimmer) / sizeof(trimmer[0])];

//led
const int led_green = 17;
const int led_yellow = 16;
const int led_red = 15;

//buzzer
const int buzzer = 2;
const int freq = 2700;
const int resolution = 8;

//pcf8563
#include "pcf8563.h"

PCF8563_Class rtc;
String rtc_date_time_old, rtc_date_time, rtc_date;

//sd_card
#include <SD.h>
//#include <SdFat.h>
//SdFat SD;
const int CS = 5;
bool avialable_sd_card = false;
int reconnect_sd_card_count = 0;

File myFile;
File record_offline_file;
File record_file;
File alarm_file;
size_t sd_result;

const int buffer_size = 30;
int record_buffer_size = 0;
int alarm_buffer_size = 0;
int record_offline_buffer_size = 0;
String record_buffer[buffer_size];
String alarm_buffer[buffer_size];
String record_offline_buffer[buffer_size];

//rom data
#include <Preferences.h>
Preferences preferences;

//etc
unsigned long display_millis_now, display_millis_old; //for oled
unsigned long sd_card_millis_now, sd_card_millis_old; //for sd_card
int display_panel = 1; //for oled
bool is_running = false; //for led
bool is_warning = true; //for led
bool is_alarm = false; //for led, buzzer
String alarm_message[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])] = {"", "", "", "", ""}; //for sd_card
unsigned long working_time_millis_now, working_time_millis_old; //reset if working over time

// Add buffer flush interval and last flush time
const unsigned long buffer_flush_interval = 5UL * 60UL * 1000UL; // 5 minutes
unsigned long last_buffer_flush_time = 0;

//mcp9600
void update_mcp9600_value() {
  update_trimmer_value();

  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    mcp9600_value[i] = mcp[i].readThermocouple() + trimmer_value[i];
  }

  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    if (mcp9600_value[i] > mcp9600_max_limit[i]) {
      alarm_message[i] = mcp9600_data_name[i] + " : " + String(mcp9600_value[i]) + " °C is over max limit at " + String(mcp9600_max_limit[i]) + " °C";
      mcp9600_alarm[i] = true;
    } else if (mcp9600_value[i] < mcp9600_min_limit[i]) {
      alarm_message[i] = mcp9600_data_name[i] + " : " + String(mcp9600_value[i]) + " °C is under min limit at " + String(mcp9600_min_limit[i]) + " °C";
      mcp9600_alarm[i] = true;
    } else {
      mcp9600_alarm[i] = false;
    }
  }
}

//trimmer
void update_trimmer_value() {
  for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
    for (int j = 0; j < trimmer_averaging; j++) {
      trimmer_value[i] = (((((constrain(analogRead(trimmer[i]) - min_trimmer_value, 0, max_trimmer_value - min_trimmer_value) / (max_trimmer_value - min_trimmer_value)) * (out_max_trimmer_value - out_min_trimmer_value)) + out_min_trimmer_value) * -1) + trimmer_value[i]) / 2;
    }
  }
}

//oled
void display_1() {

  if (avialable_wifi == true) {
    display.setCursor(0, 0);
    display.println("Wifi : Connected");
    display.setCursor(0, 8);
    display.println("IP : " + WiFi.localIP().toString());
    display.setCursor(0, 16);
    display.println("RSSI : " + String(WiFi.RSSI()) + " dBm");
  } else {
    display.setCursor(0, 0);
    display.println("Wifi : Disconnected");
    display.setCursor(0, 8);
    display.println("IP : XXX.XXX.XXX.XXX");
    display.setCursor(0, 16);
    display.println("RSSI : XXX dBm");
  }
  display.setCursor(0, 24);
  display.println("Name : " + String(host_name));
}

void display_2() {
  display.setCursor(0, 0);
  display.println("CH 1 Temp : " + String(mcp9600_value[0]) + " C");
  display.setCursor(0, 8);
  display.println("CH 2 Temp : " + String(mcp9600_value[1]) + " C");
  display.setCursor(0, 16);
  display.println("CH 3 Temp : " + String(mcp9600_value[2]) + " C");
  display.setCursor(0, 24);
  display.println("CH 4 Temp : " + String(mcp9600_value[3]) + " C");
}

void display_3() {
  display.setCursor(0, 0);
  display.println("CH 5 Temp : " + String(mcp9600_value[4]) + " C");
}

//led
void update_led_status() {
  digitalWrite(led_green, is_running);
  digitalWrite(led_yellow, is_warning);
  digitalWrite(led_red, is_alarm);
}

//rom record
void update_record_rom() {
  preferences.begin("record", false);
  preferences.putString("rtc_date_time", rtc_date_time);
  preferences.end();
}

//rom data
void update_alarm_rom() {
  preferences.begin("alarm", false);
  preferences.putBool("is_running", is_running);
  preferences.putBool("is_warning", is_warning);
  preferences.putBool("is_alarm", false);
  //  preferences.putBool("is_alarm", is_alarm);
  preferences.end();
}

//buzzer
void update_buzzer_status() {
  if (is_alarm) {
    ledcWrite(0, 150);
  } else {
    ledcWrite(0, 0);
  }
}

//pcf8563
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

String rtc_get_date() {
  RTC_Date date = rtc.getDateTime();
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02d_%02d_%04d", date.month, date.day, date.year, date.hour, date.minute, date.second);
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

//sd_card
void write_file(const char * path, const char * message) {
  myFile = SD.open(path, FILE_WRITE);

  if (!myFile) {
    Serial.println("File not found or failed to open : " + String(path));
    myFile = SD.open(path, FILE_WRITE);
    if (!myFile) {
      Serial.println("Still failed to open : " + String(path));
      return;
    } else {
      Serial.println("Created new file : " + String(path));
    }
  }

  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close();
    Serial.println("completed.");
  } else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}


void read_file(const char * path) {
  myFile = SD.open(path);
  if (!myFile) {
    Serial.println("File not found or failed to open : " + String(path));
    myFile = SD.open(path, FILE_WRITE);
    if (!myFile) {
      Serial.println("Still failed to open : " + String(path));
      return;
    } else {
      Serial.println("Created new file : " + String(path));
    }
  }

  if (myFile) {
    Serial.printf("Reading file from %s\n", path);
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  }
  else {
    Serial.print("error opening file ");
    Serial.println(path);
  }
}

//bool more_than_zero_line(const char* path) {
//  int lines = 0;
//  File myFile = SD.open(path, FILE_READ);
//
//  if (!myFile) {
//    Serial.println("File " + String(path) + " not found...");
//    return lines;
//  }
//
//  if (myFile) {
//    const int bufferSize = 2048; // Adjust buffer size as needed
//    char buffer[bufferSize];
//
//    while (myFile.available() && lines == 0) {
//      int bytesRead = myFile.read((uint8_t*)buffer, bufferSize);
//      for (int i = 0; i < bytesRead; i++) {
//        if (buffer[i] == '\n') {
//          lines++;
//          break;
//        }
//        delay(20);
//      }
//      delay(20);
//    }
//
//    myFile.close();
//  } else {
//    Serial.println("Error opening file for counting lines");
//  }
//
//  if (lines == 0) {
//    return false;
//  }else {
//    return true;
//  }
//}

//int count_line(const char * path) {
//  int lines = 0;
//  File myFile = SD.open(path, FILE_READ);
//
//  if (!myFile) {
//    Serial.println("File " + String(path) + " not found...");
//    return lines;
//  }
//
//  if (myFile) {
//    const int bufferSize = 2048; // Adjust buffer size as needed
//    char buffer[bufferSize];
//
//    while (myFile.available()) {
//      int bytesRead = myFile.read((uint8_t*)buffer, bufferSize);
//      for (int i = 0; i < bytesRead; i++) {
//        if (buffer[i] == '\n') {
//          lines++;
//        }
//        delay(20);
//      }
//      delay(20);
//    }
//
//    myFile.close();
//  } else {
//    Serial.println("Error opening file for counting lines");
//  }
//
//  return lines;
//}

//String read_line(const char * path, int lineNumber) {
//  int currentLine = 0;
//  String result = "";
//
//  myFile = SD.open(path);
//
//  if (!myFile) {
//    Serial.println("File " + String(path) + " not found...");
//    return "";
//  }
//
//  if (myFile) {
//    while (myFile.available()) {
//      String line = myFile.readStringUntil('\n');
//      if (currentLine == lineNumber) {
//        Serial.println(line);
//        result = line;
//        delay(20);
//        break;
//      }
//      currentLine++;
//      delay(20);
//    }
//    myFile.close();
//  } else {
//    Serial.println("error opening file for reading line");
//  }
//
//  return result;
//}

//void write_line(const char * path, const char * message, int lineNumber) {
//  File tempFile = SD.open("/temporary.txt", FILE_WRITE);
//  myFile = SD.open(path);
//
//  if (!myFile) {
//    Serial.println("File " + String(path) + " not found...");
//    return;
//  }
//
//  int currentLine = 0;
//  if (myFile && tempFile) {
//    while (myFile.available()) {
//      String line = myFile.readStringUntil('\n');
//      if (currentLine == lineNumber) {
//        tempFile.println(message);
//      } else {
//        tempFile.println(line);
//      }
//      currentLine++;
//    }
//    if (currentLine <= lineNumber) {
//      tempFile.println(message);
//    }
//    myFile.close();
//    tempFile.close();
//    SD.remove(path);
//    SD.rename("/temporary.txt", path);
//  } else {
//    Serial.println("error opening file for writing line");
//  }
//}

void insert_line(const char * path, const char * message) {
  myFile = SD.open(path, FILE_APPEND);

  if (!myFile) {
    Serial.println("File not found or failed to open : " + String(path));
    myFile = SD.open(path, FILE_WRITE);
    if (!myFile) {
      Serial.println("Still failed to open : " + String(path));
      return;
    } else {
      Serial.println("Created new file : " + String(path));
    }
  }

  if (myFile.println(message)) {
    // Success - flush and close
    myFile.flush();
    myFile.close();
  } else {
    Serial.println("Error writing to file");
    myFile.close();
  }

  //  if (myFile) {
  //    // Move to the end of the file
  //    myFile.seek(myFile.size());
  //
  //    // Write the new line
  //    myFile.print("\n"); // Add a newline before the new message
  //
  //    // Break the message into smaller chunks and write them
  //    const int chunkSize = 64; // Adjust chunk size as needed
  //    int messageLength = strlen(message);
  //    int offset = 0;
  //
  //    while (offset < messageLength) {
  //      int chunkLength = min(chunkSize, messageLength - offset);
  //      myFile.write((const uint8_t*)(message + offset), chunkLength); // Cast to const uint8_t*
  //      offset += chunkLength;
  //    }
  //
  //    // Flush the data to ensure it's written to the file
  //    myFile.flush();
  //
  //    // Close the file
  //    myFile.close();
  //  } else {
  //    Serial.println("Error opening file for inserting line");
  //  }
}

void remove_line(const char * path, int lineNumber) {
  File tempFile = SD.open("/temporary.txt", FILE_WRITE);
  myFile = SD.open(path);

  if (!myFile) {
    Serial.println("File not found or failed to open : " + String(path));
    myFile = SD.open(path, FILE_WRITE);
    if (!myFile) {
      Serial.println("Still failed to open : " + String(path));
      return;
    } else {
      Serial.println("Created new file : " + String(path));
    }
  }

  int currentLine = 0;
  if (myFile && tempFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine != lineNumber) {
        tempFile.println(line);
      }
      currentLine++;
      delay(10);
    }
    myFile.close();
    tempFile.close();
    SD.remove(path);
    SD.rename("/temporary.txt", path);
  } else {
    Serial.println("error opening file for removing line");
  }
}

void remove_file(const char * path) {
  if (SD.exists(path)) {
    SD.remove(path);
    Serial.printf("File %s removed successfully.\n", path);
  } else {
    Serial.printf("File %s does not exist.\n", path);
  }
  delay(10);
}

void store_data_to_sd_card() {
  long millis_sd, millis_sd_old = millis();
  
  if (avialable_sd_card == false) {
    Serial.println("Not found SD card");
    return;
  } else {
    rtc_date = rtc_get_date();

    //write record data to sd card
    String record_data = "{date : \"" + rtc_date_time + "\", machine_name : \"" + host_name + "\", data : [";
    for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
      record_data += "{name : \"" + mcp9600_data_name[i] + "\", value : " + String(mcp9600_value[i], 2) + ", min_limit : " + String(mcp9600_min_limit[i], 2) + ", max_limit : " + String(mcp9600_max_limit[i], 2) + "}";
      if (i < sizeof(mcp) / sizeof(mcp[0]) - 1) {
        record_data += ", ";
      }
    }
    record_data += "], second : " + String(rtc_get_timestamp()) + "}";

    //    insert_line(("/record_" + rtc_date + ".txt").c_str(), record_data.c_str());
    Serial.println("record_buffer before is : " + String(record_buffer_size));
    if (record_buffer_size < buffer_size - 1) {
      record_buffer[record_buffer_size] = record_data;
      record_buffer_size++;
    }else {
      record_buffer[record_buffer_size] = record_data;

      record_file = SD.open(("/record_" + rtc_date + ".txt").c_str(), FILE_APPEND);

      if (!record_file) {
        Serial.println("File not found or failed to open : record_file");
        record_file = SD.open(("/record_" + rtc_date + ".txt").c_str(), FILE_WRITE);
        if (!record_file) {
          Serial.println("Still failed to open : record_file");
        } else {
          Serial.println("Created new file : record_file");
          record_file.close();

          record_file = SD.open(("/record_" + rtc_date + ".txt").c_str(), FILE_APPEND);
        }
      }

      delay(10);
      
      for (int i = 0; i <= record_buffer_size; i++) {
        record_file.println(record_buffer[i]);
        Serial.println("store data : " + record_buffer[i]);
        delay(5);
      }

      record_file.flush();
      // record_file.close();
      record_buffer_size = 0;

      delay(10);
    }
    Serial.println("record_buffer after is : " + String(record_buffer_size));

    // sd_result = record_file.println(record_data.c_str());

    // if (sd_result == 0) {
    //   Serial.println("file : /record_" + rtc_date + ".txt | write fail!");
    //   record_file = SD.open(("/record_" + rtc_date + ".txt").c_str(), FILE_APPEND);
    //   record_file.println(record_data.c_str());
    //   record_file.flush();
    // } else {
    //   Serial.println("file : /record_" + rtc_date + ".txt | write success!");
    //   record_file.flush();
    // }

    if (avialable_wifi == false) {
      //      insert_line("/record_offline.txt", record_data.c_str());

      // sd_result = record_offline_file.println(record_data.c_str());

      // if (sd_result == 0) {
      //   Serial.println("file : /record_offline.txt | write fail!");
      //   record_offline_file = SD.open("/record_offline.txt", FILE_APPEND);
      //   record_offline_file.println(record_data.c_str());
      //   record_offline_file.flush();
      // } else {
      //   Serial.println("file : /record_offline.txt | write success!");
      //   record_offline_file.flush();
      // }

      Serial.println("record_offline_buffer before is : " + String(record_offline_buffer_size));
        if (record_offline_buffer_size < buffer_size - 1) {
          record_offline_buffer[record_offline_buffer_size] = record_data;
          record_offline_buffer_size++;
        }else {
          record_offline_buffer[record_offline_buffer_size] = record_data;

          record_offline_file = SD.open(("/record_offline_" + rtc_date + ".txt").c_str(), FILE_APPEND);

          if (!record_offline_file) {
            Serial.println("File not found or failed to open : record_offline_file");
            record_offline_file = SD.open(("/record_offline_" + rtc_date + ".txt").c_str(), FILE_WRITE);
            if (!record_offline_file) {
              Serial.println("Still failed to open : record_offline_file");
            } else {
              Serial.println("Created new file : record_offline_file");
              record_offline_file.close();
              record_offline_file = SD.open(("/record_offline_" + rtc_date + ".txt").c_str(), FILE_APPEND);
            }
          }

          delay(10);
          
          for (int i = 0; i <= record_offline_buffer_size; i++) {
            record_offline_file.println(record_offline_buffer[i]);
            Serial.println("store data : " + record_offline_buffer[i]);
            delay(5);
          }

          record_offline_file.flush();
          // record_offline_file.close();
          record_offline_buffer_size = 0;

          delay(10);
        }
        Serial.println("record_offline_buffer after is : " + String(record_offline_buffer_size));

      // delay(50);
    }

    //write alarm to sd card
    for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
      if (mcp9600_alarm[i] == true) {
        String alarm_data = "";

        if (mcp9600_value[i] > mcp9600_max_limit[i]) {
          alarm_data = "{date : \"" + rtc_date_time + "\", machine_name : \"" + host_name + "\", data_name : \"" + mcp9600_data_name[i] + "\", description : \"" + alarm_message[i] + "\", type : \"over_max_limit\", second : " + String(rtc_get_timestamp()) + "}";

          //        Serial.println("Alarm data inserted: " + alarm_data);
        } else if (mcp9600_value[i] < mcp9600_min_limit[i]) {
          alarm_data = "{date : \"" + rtc_date_time + "\", machine_name : \"" + host_name + "\", data_name : \"" + mcp9600_data_name[i] + "\", description : \"" + alarm_message[i] + "\", type : \"under_min_limit\", second : " + String(rtc_get_timestamp()) + "}";

          //          Serial.println("Alarm data inserted: " + alarm_data);
        }

        //        insert_line(("/alarm_" + rtc_date + ".txt").c_str(), alarm_data.c_str());

        // sd_result = alarm_file.println(alarm_data.c_str());

        // if (sd_result == 0) {
        //   Serial.println("file : /record_" + rtc_date + ".txt | write fail!");
        //   alarm_file = SD.open(("/alarm_" + rtc_date + ".txt").c_str(), FILE_APPEND);
        //   alarm_file.println(alarm_data.c_str());
        //   alarm_file.flush();
        // } else {
        //   Serial.println("file : /record_" + rtc_date + ".txt | write success!");
        //   alarm_file.flush();
        // }

        Serial.println("alarm_buffer before is : " + String(alarm_buffer_size));
        if (alarm_buffer_size < buffer_size - 1) {
          alarm_buffer[alarm_buffer_size] = alarm_data;
          alarm_buffer_size++;
        }else {
          alarm_buffer[alarm_buffer_size] = alarm_data;

          alarm_file = SD.open(("/alarm_" + rtc_date + ".txt").c_str(), FILE_APPEND);

          if (!alarm_file) {
            Serial.println("File not found or failed to open : alarm_file");
            alarm_file = SD.open(("/alarm_" + rtc_date + ".txt").c_str(), FILE_WRITE);
            if (!alarm_file) {
              Serial.println("Still failed to open : alarm_file");
            } else {
              Serial.println("Created new file : alarm_file");
              alarm_file.close();

              alarm_file = SD.open(("/alarm_" + rtc_date + ".txt").c_str(), FILE_APPEND);
            }
          }

          delay(10);
          
          for (int i = 0; i <= alarm_buffer_size; i++) {
            alarm_file.println(alarm_buffer[i]);
            Serial.println("store data : " + alarm_buffer[i]);
            delay(5);
          }

          alarm_file.flush();
          // alarm_file.close();
          alarm_buffer_size = 0;

          delay(10);
        }
        Serial.println("alarm_buffer after is : " + String(alarm_buffer_size));

        delay(10);
      }
    }

    millis_sd = millis();
    Serial.println(rtc_date_time + " Done to insert line using time = " + String(millis_sd - millis_sd_old));
  }
}

void flush_buffers_to_sd_card() {
  // Flush record_buffer
  if (record_buffer_size > 0) {
    String rtc_date = rtc_get_date();
    record_file = SD.open(("/record_" + rtc_date + ".txt").c_str(), FILE_APPEND);
    if (!record_file) {
      record_file = SD.open(("/record_" + rtc_date + ".txt").c_str(), FILE_WRITE);
    }
    if (record_file) {
      for (int i = 0; i < record_buffer_size; i++) {
        record_file.println(record_buffer[i]);
      }
      record_file.flush();
      record_file.close();
      record_buffer_size = 0;
    }
  }
  // Flush record_offline_buffer
  if (record_offline_buffer_size > 0) {
    String rtc_date = rtc_get_date();
    record_offline_file = SD.open(("/record_offline_" + rtc_date + ".txt").c_str(), FILE_APPEND);
    if (!record_offline_file) {
      record_offline_file = SD.open(("/record_offline_" + rtc_date + ".txt").c_str(), FILE_WRITE);
    }
    if (record_offline_file) {
      for (int i = 0; i < record_offline_buffer_size; i++) {
        record_offline_file.println(record_offline_buffer[i]);
      }
      record_offline_file.flush();
      record_offline_file.close();
      record_offline_buffer_size = 0;
    }
  }
  // Flush alarm_buffer
  if (alarm_buffer_size > 0) {
    String rtc_date = rtc_get_date();
    alarm_file = SD.open(("/alarm_" + rtc_date + ".txt").c_str(), FILE_APPEND);
    if (!alarm_file) {
      alarm_file = SD.open(("/alarm_" + rtc_date + ".txt").c_str(), FILE_WRITE);
    }
    if (alarm_file) {
      for (int i = 0; i < alarm_buffer_size; i++) {
        alarm_file.println(alarm_buffer[i]);
      }
      alarm_file.flush();
      alarm_file.close();
      alarm_buffer_size = 0;
    }
  }
}

//etc
void store_data_to_server() {
  http.begin(url);
  http.setTimeout(http_timeout);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String url_encoded_string = "machine_name=" + String(host_name);

  url_encoded_string += "&data=[";
  for (int i = 0; i < sizeof(mcp9600_address) / sizeof(mcp9600_address[0]); i++) {
    url_encoded_string += "{\"name\":\"" + mcp9600_data_name[i] + "\",\"value\":" + String(mcp9600_value[i]) + "}";
    if (i + 1 < sizeof(mcp9600_address) / sizeof(mcp9600_address[0])) {
      url_encoded_string += ",";
    }
  }
  url_encoded_string += "]";

  //  Serial.println(url + url_encoded_string);

  //  Serial.println("url_encoded_string is : " + url_encoded_string);
  int http_response_code = http.POST(url_encoded_string);

  //response monitor
  if (http_response_code > 0) {
    Serial.println("Uploaded status : " + String(http_response_code));
  } else {
    Serial.println("Upload failed : " + String(http_response_code));
  }

  http.end();
}

void store_data_from_sd_card_to_server() {
  if (avialable_sd_card == false) {
    Serial.println("Not found SD card");
    return;
  }

  // Merge new data from data.txt into upload_data.txt
  if (SD.exists("/record_offline.txt")) {
    File dataFile = SD.open("/record_offline.txt", FILE_READ);
    File uploadFile = SD.open("/upload_data.txt", FILE_APPEND);

    if (dataFile && uploadFile) {
      while (dataFile.available()) {
        String line = dataFile.readStringUntil('\n');
        uploadFile.println(line);
        delay(20);
      }
      dataFile.close();
      uploadFile.close();
      SD.remove("/record_offline.txt");  // Delete data.txt after merging
      Serial.println("Upload : Done merging data files");
    } else {
      Serial.println("Upload failed : Error merging data files");
      return;
    }
  }

  if (!SD.exists("/upload_data.txt")) {
    Serial.println("Upload failed : No upload data available");
    return;
  }

  delay(20);

  myFile = SD.open("/upload_data.txt");
  if (myFile) {
    while (myFile.available()) {
      String raw_data = myFile.readStringUntil('\n');

      //      Serial.println("checking blank data");
      raw_data.trim();
      if (raw_data.length() == 0) {
        Serial.println("skip blank line");
      } else {
        //        Serial.println("json decoding");

        StaticJsonDocument<1024> jsonDoc;  // Allocate memory for JSON parsing

        DeserializationError error = deserializeJson(jsonDoc, raw_data);
        if (error) {
          Serial.print("Upload failed : JSON Parsing failed: ");
          Serial.println(error.c_str());
          continue;
        }

        if (jsonDoc.containsKey("machine_name") && jsonDoc.containsKey("data") && jsonDoc.containsKey("second")) {
          String data_str;
          JsonArray data_array = jsonDoc["data"].as<JsonArray>();
          for (JsonObject obj : data_array) {
            if (!data_str.isEmpty()) data_str += ",";  // Separate values with a comma
            data_str += "{\"name\":\"" + obj["name"].as<String>() + "\",\"value\":" + String(obj["value"].as<float>()) +
                        ",\"min_limit\":" + String(obj["min_limit"].as<float>()) + ",\"max_limit\":" + String(obj["max_limit"].as<float>()) + "}";
          }
          //          data_str.replace("\"", "\\\"");

          String url_encoded_string = "machine_name=" + String(jsonDoc["machine_name"].as<const char*>()) +
                                      "&data=[" + data_str + "]" +
                                      "&time_stamp=" + String(jsonDoc["second"].as<long>());

          //            Serial.println("url_encoded_string is : " + url_encoded_string);

          http.begin(url);
          http.setTimeout(http_timeout);
          http.addHeader("Content-Type", "application/x-www-form-urlencoded");

          int http_response_code = http.POST(url_encoded_string);

          if (http_response_code > 0) {
            Serial.println("Uploaded status : " + String(http_response_code));
          } else {
            Serial.println("Upload failed : " + String(http_response_code));
          }

          http.end();
        } else {
          Serial.println("Upload failed : Missing keys in JSON");
        }
      }

      delay(1);
    }
    myFile.close();
    remove_file("/upload_data.txt");
    remove_file("/record_offline.txt");

    //    SD.remove("/upload.txt");

    //    if (SD.exists("/upload_datas.txt")) {
    //      if (SD.rename("/upload_datas.txt", "/upload.txt")) {
    //        Serial.println("Renamed upload_datas.txt to upload.txt");
    //      } else {
    //        Serial.println("Rename failed");
    //      }
    //    } else {
    //      Serial.println("upload_datas.txt not found");
    //    }
  } else {
    Serial.println("error opening file for reading line");
  }

  Serial.println("Upload process completed");
}

void update_status() {
  bool temp_is_warning = false; //for temporary led
  bool temp_is_alarm = false; //for temporary led, buzzer

  temp_is_warning = temp_is_warning || !avialable_sd_card;
  temp_is_warning = temp_is_warning || !avialable_wifi;

  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    temp_is_alarm = temp_is_alarm || mcp9600_alarm[i];
  }

  is_warning = temp_is_warning;

  if (is_alarm == false) {
    is_alarm = temp_is_alarm;
  }
  //  Serial.println(String(avialable_sd_card) + " : " + String(is_running) + " : " + String(is_warning) + " : " + String(is_alarm));
}

void core_0(void *not_use) {
  while (true) {
    working_time_millis_now = millis();
    display_millis_now = millis();
    sd_card_millis_now = millis();


    //set reset 1 hr time and reset when free heap is least than some number to prevent esp not response
    //    if (ESP.getFreeHeap() < 50000 || working_time_millis_now - working_time_millis_old >= 60000) {
    //      ESP.restart();
    //    }

    update_mcp9600_value();
    update_status();
    update_led_status();
    update_buzzer_status();
    update_alarm_rom();

    //store data to sd card
    rtc_date_time = rtc_get_date_time();
    if (sd_card_millis_now - sd_card_millis_old >= 500 && avialable_sd_card == true && rtc_date_time_old != rtc_date_time) {
      store_data_to_sd_card();
      sd_card_millis_old = sd_card_millis_now;
      rtc_date_time_old = rtc_date_time;
    }

    // Periodically flush buffers to SD card even if not full
    if (millis() - last_buffer_flush_time >= buffer_flush_interval) {
      flush_buffers_to_sd_card();
      last_buffer_flush_time = millis();
    }

    //switch display
    if (display_panel == 0) {
      if (display_millis_now - display_millis_old >= 1000) {
        display_panel += 1;
        display_millis_old = display_millis_now;
      }
    } else {
      if (display_millis_now - display_millis_old >= 5000) {
        //        Serial.println("count line is : " + String(count_line("/data.txt")));
        display_panel += 1;
        display_millis_old = display_millis_now;

        //        if (!SD.begin(CS)) {
        //          avialable_sd_card = false;
        //        }else {
        //          avialable_sd_card = true;
        //        }

        //        if (avialable_sd_card == true) {
        //          myFile = SD.open("/data.txt");
        //          if (myFile) {
        //            myFile.close();
        //          }else {
        //            ESP.restart(); //reset esp to clear cache sd card alway can detect whenever sd card is uninstalled for prepare SD card that reinstall for next time
        //          }
        //        }
      }
    }

    display.clearDisplay();
    if (display_panel == 1) {
      display_1();
    } else if (display_panel == 2) {
      display_2();
    } else if (display_panel == 3) {
      display_3();
    } else {
      display_panel = 0;
      //free display prevent oled display burn for run as long time
      //the display should be black (off) for a second
    }
    display.display();

    if (digitalRead(0) == LOW) {
      //unload sd card
      if (myFile) myFile.close();
      if (record_file) record_file.close();
      if (alarm_file) alarm_file.close();
      if (record_offline_file) record_offline_file.close();
      SD.end();
  
      while(true) {
        digitalWrite(led_green, HIGH);
        digitalWrite(led_yellow, HIGH);
        digitalWrite(led_red, HIGH);
        delay(500);
        digitalWrite(led_green, LOW);
        digitalWrite(led_yellow, LOW);
        digitalWrite(led_red, LOW);
        delay(500);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void core_1(void *not_use) {
  while (true) {
    Serial.printf("Total heap: %d Free heap: %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
    delay(500);

    if (WiFi.status() != WL_CONNECTED) {
      avialable_wifi = false;

      WiFi.begin(ssid, password);
      Serial.println("Connecting to WiFi...");
      while (WiFi.status() != WL_CONNECTED) {
        delay(10);
        //        Serial.print(".");
        reconnect_wifi_count++;
        if (reconnect_wifi_count >= 1000) {
          //      ESP.restart();
          reconnect_wifi_count = 0;
          break;
        }
      }

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("fail connection");
        avialable_wifi = false;
      } else {
        Serial.println("done connection");
        avialable_wifi = true;
      }
    } else {
      avialable_wifi = true;
      ArduinoOTA.handle();
    }

    if (avialable_wifi == true) {
      //store data to server
      store_data_to_server();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(2000000);

  //for mcp9600, sd_card
  while (!Serial) {
    delay(10);
  }

  //rom data
  preferences.begin("alarm", false);
  is_running = preferences.getBool("is_running", false);
  is_warning = preferences.getBool("is_warning", false);
  is_alarm = preferences.getBool("is_alarm", false);
  preferences.end();

  //boot button
  pinMode(0, INPUT_PULLUP);

  //trimmer
  for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
    pinMode(trimmer[i], INPUT);
  }

  //led
  pinMode(led_green, OUTPUT);
  pinMode(led_yellow, OUTPUT);
  pinMode(led_red, OUTPUT);

  //buzzer
  ledcSetup(0, freq, resolution); //2.7kHz, 8-bit resolution 0-255
  ledcAttachPin(buzzer, 0);

  //init oled
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0x55);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Strating...");

  display.setCursor(0, 8);
  display.println("SD card...");
  display.display();

  //sd_card
  while (!SD.begin(CS)) {
    Serial.println("SD card initialization failed! Retrying...");
    delay(1000);
    reconnect_sd_card_count++;

    if (reconnect_sd_card_count > 20) {
      reconnect_sd_card_count = 0;
      break;
    }
  }

  if (!SD.begin(CS)) {
    Serial.println("SD card initialization fail.");
    display.setCursor(0, 8);
    display.println("SD card...fail");

    avialable_sd_card = false;
  } else {
    Serial.println("SD card initialization done.");
    display.setCursor(0, 8);
    display.println("SD card...done");

    avialable_sd_card = true;
  }

  display.setCursor(0, 16);
  display.println("Wi-Fi...");
  display.display();

  //wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    //    Serial.print(".");
    reconnect_wifi_count++;
    if (reconnect_wifi_count >= 1000) {
      //      ESP.restart();
      reconnect_wifi_count = 0;
      break;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("fail connection");
    avialable_wifi = false;

    display.setCursor(0, 16);
    display.println("Wi-Fi...fail");
  } else {
    Serial.println("done connection");
    avialable_wifi = true;

    display.setCursor(0, 16);
    display.println("Wi-Fi...done");
  }

  //init mcp9600
  for (uint8_t i = 0; i < sizeof(mcp9600_address); i++) {
    if (mcp[i].begin(mcp9600_address[i])) {
      Serial.print("**********Found MCP9600 at address 0x");
      Serial.print(mcp9600_address[i], HEX);
      Serial.println("**********");

      mcp[i].setAmbientResolution(ambientRes);
      Serial.print("Ambient Resolution set to: ");
      switch (ambientRes) {
        case RES_ZERO_POINT_25:    Serial.println("0.25°C"); break;
        case RES_ZERO_POINT_125:   Serial.println("0.125°C"); break;
        case RES_ZERO_POINT_0625:  Serial.println("0.0625°C"); break;
        case RES_ZERO_POINT_03125: Serial.println("0.03125°C"); break;
      }

      mcp[i].setADCresolution(MCP9600_ADCRESOLUTION_18);
      Serial.print("ADC resolution set to ");
      switch (mcp[i].getADCresolution()) {
        case MCP9600_ADCRESOLUTION_18:   Serial.print("18"); break;
        case MCP9600_ADCRESOLUTION_16:   Serial.print("16"); break;
        case MCP9600_ADCRESOLUTION_14:   Serial.print("14"); break;
        case MCP9600_ADCRESOLUTION_12:   Serial.print("12"); break;
      }
      Serial.println(" bits");

      mcp[i].setThermocoupleType(MCP9600_TYPE_K);
      Serial.print("Thermocouple type set to ");
      switch (mcp[i].getThermocoupleType()) {
        case MCP9600_TYPE_K:  Serial.print("K"); break;
        case MCP9600_TYPE_J:  Serial.print("J"); break;
        case MCP9600_TYPE_T:  Serial.print("T"); break;
        case MCP9600_TYPE_N:  Serial.print("N"); break;
        case MCP9600_TYPE_S:  Serial.print("S"); break;
        case MCP9600_TYPE_E:  Serial.print("E"); break;
        case MCP9600_TYPE_B:  Serial.print("B"); break;
        case MCP9600_TYPE_R:  Serial.print("R"); break;
      }
      Serial.println(" type");

      mcp[i].enable(true);
    } else {
      Serial.print("Notfound MCP9600 at address 0x");
      Serial.println(mcp9600_address[i], HEX);
      Serial.println("Check wiring!");
      while (1);
    }
  }

  //pcf8563
  rtc.begin();
//  rtc.setDateTime(2025, 6, 8, 14 - 7, 43, 00);
  //
  //  while(true) {
  //    delay(100);
  //  }

  is_running = true;

  display.setCursor(0, 24);
  display.println("Uploading data...");
  display.display();

  if (avialable_wifi == true) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Uploading data...");
    display.display();
    store_data_from_sd_card_to_server();
  }

  //start OTA upload
  ArduinoOTA.setHostname(host_name);

  ArduinoOTA
  .onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else
      type = "filesystem";
    Serial.println("Start updating " + type);
  })
  .onEnd([]() {
    Serial.println("\nEnd");
  })
  .onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  })
  .onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });

  ArduinoOTA.begin();

  display.setCursor(0, 24);
  display.println("Uploading data...done");
  display.display();

  delay(1000);

  display.clearDisplay();

  working_time_millis_now = millis();
  display_millis_now = millis();
  sd_card_millis_now = millis();
  working_time_millis_old = working_time_millis_now;
  display_millis_old = display_millis_now;
  sd_card_millis_old = sd_card_millis_now;

  xTaskCreatePinnedToCore(
    core_0,       // Function to implement the task
    "core_0",     // Name of the task
    10000,        // Stack size in words
    NULL,         // Task input parameter
    1,            // Priority of the task
    NULL,         // Task handle
    0             // Core ID
  );

  xTaskCreatePinnedToCore(
    core_1,       // Function to implement the task
    "core_1",     // Name of the task
    10000,        // Stack size in words
    NULL,         // Task input parameter
    1,            // Priority of the task
    NULL,         // Task handle
    1             // Core ID
  );
}

void loop() {
}