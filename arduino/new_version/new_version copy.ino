#include <SPI.h> //for oled, sd_card
#include <Wire.h> //for oled, mcp9600, pcf8563

//wifi
#include <WiFi.h>

const char* ssid = "MrSuradechTH";
const char* password = "1234567890";
int reconnect_wifi_count = 0;

//http
#include <HTTPClient.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

HTTPClient http;

const char* url = "http://192.168.6.39:8005/record/add";
const int http_timeout = 800;

//json
#include <ArduinoJson.h>

//ota
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* host_name = "DEMO_ Machine";

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
float mcp9600_max_limit[] = {40.00, 40.00, 40.00, 40.00, 40.00};
float mcp9600_min_limit[] = {-10.00, -10.00, -10.00, -10.00, -10.00};
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
const float out_max_trimmer_value = 0.50;
const float out_min_trimmer_value = -0.50;
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

//sd_card
#include <SD.h>

File myFile;
const int CS = 5;

//etc
unsigned long display_millis_now,display_millis_old; //for oled
unsigned long sd_card_millis_now,sd_card_millis_old; //for sd_card
int display_panel = 1; //for oled
bool is_running = false; //for led
bool is_warning = false; //for led
bool is_alarm = false; //for led, buzzer, sd_card
String alarm_message[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])] = {"", "", "", "", ""}; //for sd_card

//mcp9600
void update_mcp9600_value() {
  update_trimmer_value();
  
  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    mcp9600_value[i] = mcp[i].readThermocouple() + trimmer_value[i];
    // Serial.println("CH " + String(i) + " : " + String(mcp9600_value[i]) + " °C [" + String(mcp[i].readThermocouple()) + "] [" + String(trimmer_value[i]) + "]");
  }

  check_alarm();

//  for (uint8_t i = 0; i < sizeof(mcp9600_address); i++) {
//    Serial.println("CH " + String(i) + " : " + String(mcp9600_value[i]) + " °C");
//  }
}

//trimmer
void update_trimmer_value() {
  for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
    for (int j = 0; j < trimmer_averaging; j++) {
      trimmer_value[i] = (((((constrain(analogRead(trimmer[i]) - min_trimmer_value, 0, max_trimmer_value - min_trimmer_value) / (max_trimmer_value - min_trimmer_value)) * (out_max_trimmer_value - out_min_trimmer_value)) + out_min_trimmer_value) * -1) + trimmer_value[i]) / 2;
    }
  }

//  for (int i = 0; i < sizeof(trimmer) / sizeof(trimmer[0]); i++) {
//      Serial.println("Trimmer[" + String(i) + "] value : " + String(trimmer_value[i], 2));
//  }
}

//oled
void display_1() {
  display.setCursor(0, 0);
  display.println("CH 1 Temp : " + String(mcp9600_value[0]) + " C");
  display.setCursor(0, 8);
  display.println("CH 2 Temp : " + String(mcp9600_value[1]) + " C");
  display.setCursor(0, 16);
  display.println("CH 3 Temp : " + String(mcp9600_value[2]) + " C");
  display.setCursor(0, 24);
  display.println("CH 4 Temp : " + String(mcp9600_value[3]) + " C");
}

void display_2() {
  display.setCursor(0, 0);
  display.println("CH 5 Temp : " + String(mcp9600_value[4]) + " C");
}

//led
void update_led_status() {
  digitalWrite(led_green, is_running);
  digitalWrite(led_yellow, is_warning);
  digitalWrite(led_red, is_alarm);
}

//buzzer
void update_buzzer_status() {
  if (is_alarm) {
    ledcWrite(0, 150);
  }else {
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

unsigned long rtc_get_timestamp() {
  RTC_Date date = rtc.getDateTime();
  return rtc_date_to_timestamp(date);
}

void rtc_show_date_time() {
  RTC_Date date = rtc.getDateTime();
  Serial.printf("%02d/%02d/%04d %02d:%02d:%02d\n", date.month, date.day, date.year, date.hour, date.minute, date.second);
}

//sd_card
void write_file(const char * path, const char * message){
  myFile = SD.open(path, FILE_WRITE);
  if (myFile) {
    Serial.printf("Writing to %s ", path);
    myFile.println(message);
    myFile.close();
    Serial.println("completed.");
  }else {
    Serial.println("error opening file ");
    Serial.println(path);
  }
}


void read_file(const char * path){
  myFile = SD.open(path);
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

int count_line(const char * path) {
  int lines = 0;

  if (!SD.exists(path)) {
    return lines;
  }
  
  myFile = SD.open(path);
  if (myFile) {
    while (myFile.available()) {
      if (myFile.read() == '\n') {
        lines++;
      }
    }
    myFile.close();
  } else {
    Serial.println("error opening file for counting lines");
  }
  return lines;
}

void read_line(const char * path, int lineNumber) {
  int currentLine = 0;
  myFile = SD.open(path);
  if (myFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine == lineNumber) {
        Serial.println(line);
        break;
      }
      currentLine++;
    }
    myFile.close();
  } else {
    Serial.println("error opening file for reading line");
  }
}

void write_line(const char * path, const char * message, int lineNumber) {
  File tempFile = SD.open("/temporary.txt", FILE_WRITE);
  myFile = SD.open(path);
  int currentLine = 0;
  if (myFile && tempFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine == lineNumber) {
        tempFile.println(message);
      } else {
        tempFile.println(line);
      }
      currentLine++;
    }
    if (currentLine <= lineNumber) {
      tempFile.println(message);
    }
    myFile.close();
    tempFile.close();
    SD.remove(path);
    SD.rename("/temporary.txt", path);
  } else {
    Serial.println("error opening file for writing line");
  }
}

void insert_line(const char * path, const char * message) {
  myFile = SD.open(path, FILE_WRITE);

  if (!myFile) {  
    Serial.println("File not found, creating file...");
    myFile = SD.open(path, FILE_WRITE);
  }
  
  if (myFile) {
    myFile.seek(myFile.size());
    myFile.print("\n");
    myFile.print(message);
    myFile.close();
  } else {
    Serial.println("error opening file for inserting line");
  }
}

void remove_line(const char * path, int lineNumber) {
  File tempFile = SD.open("/temporary.txt", FILE_WRITE);
  myFile = SD.open(path);
  int currentLine = 0;
  if (myFile && tempFile) {
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (currentLine != lineNumber) {
        tempFile.println(line);
      }
      currentLine++;
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
}

void store_data_to_sd_card() {
  if (!SD.begin(CS)) {
    Serial.println("Not found SD card");
    return;
  }

  //write record data to sd card
  String record_data = "{date : \"" + rtc_get_date_time() + "\", machine_id : 1, machine_name : \"machine_name\", data : [";
  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    record_data += "{name : \"" + mcp9600_data_name[i] + "\", value : " + String(mcp9600_value[i], 2) + ", min_limit : " + String(mcp9600_min_limit[i], 2) + ", max_limit : " + String(mcp9600_max_limit[i], 2) + "}";
    if (i < sizeof(mcp) / sizeof(mcp[0]) - 1) {
      record_data += ", ";
    }
  }
  record_data += "], second : " + String(rtc_get_timestamp()) + "}";

//  Serial.println("Record data inserted: " + record_data);

  insert_line("/data.txt", record_data.c_str());

  //write alarm to sd card
  if (is_alarm) {
    for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
      if (mcp9600_alarm[i] == true) {
        if (mcp9600_value[i] > mcp9600_max_limit[i]) {
          String alarm_data = "{date : \"" + rtc_get_date_time() + "\", machine_id : 1, machine_name : \"machine_name\", data_name : \"" + mcp9600_data_name[i] + "\", description : \"" + alarm_message[i] + "\", type : \"over_max_limit\", second : " + String(rtc_get_timestamp()) + "}";
          
          insert_line("/alarm.txt", alarm_data.c_str());
          
  //        Serial.println("Alarm data inserted: " + alarm_data);
        } else if (mcp9600_value[i] < mcp9600_min_limit[i]) {
          String alarm_data = "{date : \"" + rtc_get_date_time() + "\", machine_id : 1, machine_name : \"machine_name\", data_name : \"" + mcp9600_data_name[i] + "\", description : \"" + alarm_message[i] + "\", type : \"under_min_limit\", second : " + String(rtc_get_timestamp()) + "}";
          
          insert_line("/alarm.txt", alarm_data.c_str());

  //        Serial.println("Alarm data inserted: " + alarm_data);
        }
      }
    }
  }
}

// void store_data_to_sd_card() {
//   String data = "{date : \"" + rtc_get_date_time() + "\", machine_id : 1, machine_name : \"machine_name\", data : [";
//   for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
//     data += "{name : \"test" + String(i) + "\", value : " + String(mcp9600_value[i], 2) + ", min_limit : " + String(mcp9600_min_limit[i], 2) + ", max_limit : " + String(mcp9600_max_limit[i], 2) + "}";
//     if (i < sizeof(mcp) / sizeof(mcp[0]) - 1) {
//       data += ", ";
//     }
//   }
//   data += "], second : " + String(rtc_get_timestamp()) + "}";

//   int maxLine = count_line("/data.txt");
  
//   if (maxLine >= 0) {
//     write_line("/data.txt", data.c_str(), maxLine + 1);
//   } else {
//     write_file("/data.txt", data.c_str());
//   }
// }

void reset_alarm() {
  is_alarm = false;

  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    alarm_message[i] = "";
    mcp9600_alarm[i] = false;
  }
}

void check_alarm() {
  is_alarm = false;

  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    if (mcp9600_value[i] > mcp9600_max_limit[i]) {
      is_alarm = true;
      alarm_message[i] = mcp9600_data_name[i] + " : " + String(mcp9600_value[i]) + " °C is over max limit at " + String(mcp9600_max_limit[i]) + " °C";
      mcp9600_alarm[i] = true;
    }else if (mcp9600_value[i] < mcp9600_min_limit[i]) {
      is_alarm = true;
      alarm_message[i] = mcp9600_data_name[i] + " : " + String(mcp9600_value[i]) + " °C is under min limit at " + String(mcp9600_min_limit[i]) + " °C";
      mcp9600_alarm[i] = false;
    }else {
      mcp9600_alarm[i] = false;
    }
  }
}

void store_data_to_server() {
  http.begin(url);
  http.setTimeout(http_timeout);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String url_encoded_string = "machine_name=" + String(host_name) + "&time_stamp=" + String(rtc_get_timestamp());

  url_encoded_string += "&data=[";
  for (int i = 0; i < sizeof(mcp9600_address) / sizeof(mcp9600_address[0]); i++) {
      url_encoded_string += "{\"name\":\"" + mcp9600_data_name[i] + "\",\"value\":" + String(mcp9600_value[i]) + "}";
      if (i + 1 < sizeof(mcp9600_address) / sizeof(mcp9600_address[0])) {
        url_encoded_string += ",";
      }
  }
  url_encoded_string += "]";

//  Serial.println(url + url_encoded_string);
                            
  int http_response_code = http.POST(url_encoded_string);

  //response monitor
//  if (http_response_code > 0) {
//      Serial.print("HTTP Response Code: ");
//      Serial.println(http_response_code);
//      String response = http.getString();
//      Serial.println("Server Response: " + response);
//  } else {
//      Serial.print("Error on sending POST: ");
//      Serial.println(http_response_code);
//  }

  http.end();
}

void store_data_from_sd_card_to_server() {
  int data_line_count = count_line("/data.txt");

  for (int i = 0;i < data_line_count;i++) {
    
  }
}

void core_0(void *not_use) {
  while(true) {
    display_millis_now = millis();
    sd_card_millis_now = millis();
    
    update_mcp9600_value();
    update_led_status();
    update_buzzer_status();

    //store data to sd card
    if (sd_card_millis_now - sd_card_millis_old >= 1000 && WiFi.status() != WL_CONNECTED) {
      store_data_to_sd_card();
      sd_card_millis_old = sd_card_millis_now;
    }

    //switch display
    if (display_panel == 0) {
      if (display_millis_now - display_millis_old >= 1000) {
        display_panel += 1;
        display_millis_old = display_millis_now;
      }
    }else {
      if (display_millis_now - display_millis_old >= 5000) {
        display_panel += 1;
        display_millis_old = display_millis_now;
    //    Serial.println("***** show data record ***");
    //    read_file("/data.txt");
      }
    }
  
    display.clearDisplay();
    if (display_panel == 1) {
      display_1();
    }else if (display_panel == 2) {
      display_2();
    }else {
      display_panel = 0;
      //free display prevent oled display burn for run as long time
      //the display should be black (off) for a second
    }
    display.display();
  
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void core_1(void *not_use) {
  while(true) {
    Serial.printf("Total heap: %d Free heap: %d\n", ESP.getHeapSize(), ESP.getFreeHeap());
    delay(500);

    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(ssid, password);
      Serial.println("Connecting to WiFi...");
      while (WiFi.status() != WL_CONNECTED) {
        delay(10);
        Serial.print(".");
        reconnect_wifi_count++;
        if (reconnect_wifi_count >= 1000) {
    //      ESP.restart();
          reconnect_wifi_count = 0;
          break;
        }
      }

      if (WiFi.status() == WL_CONNECTED) {
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
      }
    }

    if (WiFi.status() == WL_CONNECTED) {
      //store data to server
      store_data_to_server();
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(2000000);

  //wifi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.print(".");
    reconnect_wifi_count++;
    if (reconnect_wifi_count >= 1000) {
//      ESP.restart();
      reconnect_wifi_count = 0;
      break;
    }
  }

  //for mcp9600, sd_card
  while (!Serial) {
    delay(10);
  }

  //sd_card
//  while (!SD.begin(CS)) {
//    Serial.println("SD card initialization failed! Retrying...");
//    delay(1000);
//  }
//  Serial.println("SD card initialization done.");

  if (!SD.begin(CS)) {
    Serial.println("SD card initialization failed! Retrying...");
    delay(1000);
    if (!SD.begin(CS)) {
      Serial.println("SD card initialization fail.");
    }
  }else {
    Serial.println("SD card initialization done.");
  }
  
  //init oled
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(0x55);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("Strating...");
  display.display();

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
    }else {
      Serial.print("Notfound MCP9600 at address 0x");
      Serial.println(mcp9600_address[i], HEX);
      Serial.println("Check wiring!");
      while (1);
    }
  }

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

  //pcf8563
  rtc.begin();
//  rtc.setDateTime(2025, 2, 17, 12, 34, 00);
//
//  while(true) {
//    delay(100);
//  }
  
  display.clearDisplay();

  is_running = true;

  display_millis_now = millis();
  sd_card_millis_now = millis();
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

//  remove_file("/data.txt");
//  write_file("/data.txt", "");
}

void loop() {
}