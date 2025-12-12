#include <Arduino.h>
#include <painlessMesh.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#define   MESH_PREFIX     "CrowdMesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555
#define   SYSTEM_ID       0x01

const char* AP_ssid = "ClientNode_AP";
const char* AP_password = "clientnodepass";
const char* external_ip = "10.78.49.2";
const uint16_t flask_port = 5000;

Scheduler userScheduler;
painlessMesh mesh;
HTTPClient http;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String imageBuffer = "";
size_t currentImgChunk = 0;
size_t totalImgChunks = 0;
String latestImage = "";

void receivedCallback(uint32_t from, String &msg) {
  if(msg.startsWith("IMG:")) {
    int idx1 = msg.indexOf(':', 4);
    int idx2 = msg.indexOf(':', idx1 + 1);
    size_t chunk = msg.substring(4, idx1).toInt();
    size_t total = msg.substring(idx1 + 1, idx2).toInt();
    String data = msg.substring(idx2 + 1);
    if(chunk == 0) {
      imageBuffer = data;
      currentImgChunk = 0;
      totalImgChunks = total;
      Serial.printf("[CENTRAL] Started receiving image: %d chunks\n", total);
    } else if(chunk == currentImgChunk + 1) {
      imageBuffer += data;
      currentImgChunk = chunk;
    }
    if(currentImgChunk == totalImgChunks - 1) {
      latestImage = imageBuffer;
      imageBuffer = "";
      Serial.printf("[CENTRAL] Image received successfully: %d bytes\n", latestImage.length());
    }
  }
}

void sendDataTest(void* params) {
  while (true) {
    if (latestImage.length() > 0) {
      Serial.printf("[CENTRAL] Sending image to Flask: %d bytes\n", latestImage.length());
      String jsonBody = "{";
      jsonBody += "\"image\":\"" + latestImage + "\",";
      jsonBody += "\"peopleCount\":0";
      jsonBody += "}";
      String url = String("http://") + external_ip + ":" + String(flask_port) + "/api/systems/" + String(SYSTEM_ID);
      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.sendRequest("PATCH", jsonBody);
      if(httpResponseCode > 0) {
        Serial.printf("[CENTRAL] Image sent to Flask successfully: HTTP %d\n", httpResponseCode);
      } else {
        Serial.printf("[CENTRAL] Failed to send image to Flask: HTTP %d\n", httpResponseCode);
      }
      http.end();
    } else {
      Serial.println("[CENTRAL] No image to send");
    }
    vTaskDelay(pdTICKS_TO_MS(2000));
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  WiFi.softAP(AP_ssid, AP_password);
  mesh.onReceive(&receivedCallback);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len){});
  server.addHandler(&ws);
  server.begin();

  xTaskCreate(sendDataTest, "SendDataTest", 4096, NULL, 1, NULL);
}

void loop() {
  mesh.update();
}
