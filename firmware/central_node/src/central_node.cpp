#include <Arduino.h>
#include <painlessMesh.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

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
int sensorPeopleCount = 0;

void receivedCallback(uint32_t from, String &msg) {
  if(msg.startsWith("{\"node\":\"sensor\"")) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    if(!error && doc.containsKey("count")) {
      sensorPeopleCount = doc["count"].as<int>();
      Serial.printf("[CENTRAL] Received count=%d\n", sensorPeopleCount);
      
      // Immediately send count update to Flask
      String jsonBody = "{\"peopleCount\":" + String(sensorPeopleCount) + "}";
      String url = String("http://") + external_ip + ":" + String(flask_port) + "/api/systems/" + String(SYSTEM_ID);
      HTTPClient tempHttp;
      tempHttp.begin(url);
      tempHttp.addHeader("Content-Type", "application/json");
      int httpResponseCode = tempHttp.sendRequest("PATCH", jsonBody);
      if(httpResponseCode > 0) {
        Serial.printf("[CENTRAL] Count update sent to Flask: %d people\n", sensorPeopleCount);
      } else {
        Serial.printf("[CENTRAL] Failed to send count update: HTTP %d\n", httpResponseCode);
      }
      tempHttp.end();
    }
  }
  // Handle image data
  else if(msg.startsWith("IMG:")) {
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
      Serial.printf("[CENTRAL] Sending data to Flask - Image: %d bytes, Count: %d\n", latestImage.length(), sensorPeopleCount);
      String jsonBody = "{";
      jsonBody += "\"image\":\"" + latestImage + "\",";
      jsonBody += "\"peopleCount\":" + String(sensorPeopleCount);
      jsonBody += "}";
      String url = String("http://") + external_ip + ":" + String(flask_port) + "/api/systems/" + String(SYSTEM_ID);
      http.begin(url);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.sendRequest("PATCH", jsonBody);
      if(httpResponseCode > 0) {
        Serial.printf("[CENTRAL] Data sent to Flask successfully: HTTP %d\n", httpResponseCode);
        
        // Parse response to get crowdLevel
        String response = http.getString();
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);
        if(!error && doc.containsKey("crowdLevel")) {
          String crowdLevel = doc["crowdLevel"].as<String>();
          Serial.printf("[CENTRAL] Received crowdLevel from Flask: %s\n", crowdLevel.c_str());
          
          // Send crowdLevel to sensor node via mesh
          JsonDocument meshDoc;
          meshDoc["node"] = "central";
          meshDoc["crowdLevel"] = crowdLevel;
          String meshMsg;
          serializeJson(meshDoc, meshMsg);
          mesh.sendBroadcast(meshMsg);
          Serial.printf("[CENTRAL] Sent crowdLevel to mesh: %s\n", meshMsg.c_str());
        }
      } else {
        Serial.printf("[CENTRAL] Failed to send data to Flask: HTTP %d\n", httpResponseCode);
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
