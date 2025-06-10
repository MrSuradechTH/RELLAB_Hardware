#include <WiFi.h>
#include <WebSocketsClient.h>

WebSocketsClient webSocket;

int LED = 2;

const char* ssid = "rellab";
const char* password = "1234567890";

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    String str = "";
    switch(type) {
      case WStype_DISCONNECTED:
        Serial.println("Websocket is disconnected");
        break;
      case WStype_CONNECTED:
        Serial.println("Websocket is connected");
        break;
      case WStype_TEXT:
        Serial.printf("Websocket get text : %s\n", payload);
//         send message to server when receive message
//        webSocket.sendTXT("Message received");
        break;
      case WStype_BIN:
        Serial.printf("Websocket get binary length : %u\n", length);
        break;
      default:
          break;
    }
}

void setup() {
  pinMode(LED, OUTPUT);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  webSocket.begin("192.168.0.100", 8080, "/");
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(10);
  Serial.println("Websocket is started");
}

void loop() {
  webSocket.loop();
  delay(500);
//  delay(100);
//  digitalWrite(LED, HIGH);
//  delay(100);
//  digitalWrite(LED, LOW);
}
