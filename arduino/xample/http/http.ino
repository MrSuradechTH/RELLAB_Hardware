#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "Unknown";
const char* password = "1234567890";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    Serial.println(".");
  }
  Serial.println("Connected to WiFi");
}

void loop() {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin("http://192.168.100.59/master.php");

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String data_string = "machine_name=BAKE_03&mode=record_update_data&data={\"temp\":20,\"data2\":80}";

    int status_code = http.POST(data_string);

    if(status_code > 0) {
      String response = http.getString();
      Serial.println(status_code);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(status_code);
    }

    http.end();
  }

  delay(500);
}
