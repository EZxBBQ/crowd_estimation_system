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

String frameBuffer = "";
size_t currentChunk = 0;
size_t totalChunks = 0;
String currentFrame = "";

void receivedCallback(uint32_t from, String &msg) {
  if(msg.startsWith("FRAME:")) {
    int idx1 = msg.indexOf(':', 6);
    int idx2 = msg.indexOf(':', idx1 + 1);
    size_t chunk = msg.substring(6, idx1).toInt();
    size_t total = msg.substring(idx1 + 1, idx2).toInt();
    String data = msg.substring(idx2 + 1);
    
    if(chunk == 0) {
      currentFrame = data;
      currentChunk = 0;
      totalChunks = total;
    } else if(chunk == currentChunk + 1) {
      currentFrame += data;
      currentChunk = chunk;
    }
    
    if(currentChunk == totalChunks - 1) {
      ws.textAll(currentFrame);
      currentFrame = "";
    }
  }
  mesh.sendSingle(from, "CENTRAL");
}

void sendDataTest(void* params)
{
  while (true)
  {
    // kirim data (dummy) ke flask server
    String dummyImage = "test3.jpg";
    int peopleCount = 50;

    String jsonBody = "{";
    jsonBody += "\"image\":\"" + dummyImage + "\",";
    jsonBody += "\"peopleCount\":" + String(peopleCount);
    jsonBody += "}";

    if (WiFi.getMode() == WIFI_AP || true) {

      String url = String("http://") + external_ip + ":" + String(flask_port) + "/api/systems/" + String(SYSTEM_ID);

      Serial.println("POST → " + url);
      Serial.println("Payload: " + jsonBody);

      http.begin(url);
      http.addHeader("Content-Type", "application/json");

      int httpResponseCode = http.sendRequest("PATCH", jsonBody);

      Serial.print("HTTP Response Code: ");
      Serial.println(httpResponseCode);

      if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server Response: " + response);
      } else {
        Serial.println("HTTP POST FAILED");
      }

      http.end();
    } 
    else {
      Serial.println("WiFi not in AP mode — cannot reach laptop");
    }
    vTaskDelay(pdTICKS_TO_MS(10000)); // delay 10 detik
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
