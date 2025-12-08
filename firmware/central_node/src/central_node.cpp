/*
 * CENTRAL NODE (ESP32 Bridge)
 * Tugas: Terima data Mesh -> Kirim ke Laptop via WiFi.
 */
#include <Arduino.h>
#include <painlessMesh.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <base64.h>

// --- KONFIGURASI MESH (Harus sama persis dengan node lain) ---
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

// Callback saat terima data dari Mesh (Sensor/Kamera)
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  // Proses data di sini (misal: simpan, tampilkan, dsb)
}

void sendDataTest(void* params)
{
  while (true)
  {
    // kirim data (dummy) ke flask server
    String dummyImage = "test2.jpeg";
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

  // Setup AP WiFi
  WiFi.mode(WIFI_AP);

  // Setup Mesh
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  WiFi.softAP(AP_ssid, AP_password);
  IPAddress IP = WiFi.softAPIP();
  Serial.println("Central Node AP IP address: " + IP.toString());
  mesh.onReceive(&receivedCallback);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);

  xTaskCreate(
    sendDataTest,
    "SendDataTest",
    4096,
    NULL,
    1,
    NULL
  );
}

void loop() {
  mesh.update();
}
