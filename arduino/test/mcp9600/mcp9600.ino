#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"

const uint8_t mcp9600_address[] = {0x67, 0x60, 0x65, 0x66, 0x64};
Adafruit_MCP9600 mcp[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])];
float mcp9600_vlaue[sizeof(mcp9600_address) / sizeof(mcp9600_address[0])];

/* Set and print ambient resolution */
Ambient_Resolution ambientRes = RES_ZERO_POINT_03125;

void update_mcp9600_value() {
  for (uint8_t i = 0; i < sizeof(mcp) / sizeof(mcp[0]); i++) {
    mcp9600_vlaue[i] = mcp[i].readThermocouple();
  }
}

void setup() {
  Serial.begin(2000000);
  while (!Serial) {
    delay(10);
  }

  //init mcp9600
  for (uint8_t i = 0; i < sizeof(mcp9600_address) / sizeof(mcp9600_address[0]); i++) {
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
}

void loop() {
  update_mcp9600_value();
  for (uint8_t i = 0; i < sizeof(mcp9600_address); i++) {
    Serial.println("CH " + String(i) + " : " + String(mcp9600_vlaue[i]) + " °C");
  }
  delay(1000);
}