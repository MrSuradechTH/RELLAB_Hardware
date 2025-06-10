#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define OLED_RESET 16
Adafruit_SSD1306 display(OLED_RESET);

void setup() {
  Serial.begin(115200);

  Serial.println("Initializing SSD1306...");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 initialization failed!");
    while (true);
  }
  Serial.println("SSD1306 initialized successfully!");

  delay(100);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("TESTER");
  display.setCursor(0, 7);
  display.println("MrSuradechTH");
  display.display();
  Serial.println("Display updated successfully!");
}

void loop() {
  delay(1000);
}
