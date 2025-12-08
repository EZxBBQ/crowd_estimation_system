#include <Arduino.h>
#include "esp_camera.h"
#include <painlessMesh.h>
#include <base64.h>
#include <WiFi.h>
#include <WebServer.h>

#define   MESH_PREFIX     "CrowdMesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555
#define   FRAME_DELAY     100
#define   CAMERA_MODEL_AI_THINKER

// WiFi credentials for web viewing
#define WIFI_SSID "ESP32-CAM"
#define WIFI_PASS "12345678" 

Scheduler userScheduler;
painlessMesh mesh;
uint32_t centralNodeId = 0;
unsigned long lastTime = 0;
WebServer server(80);

// --- PIN DEFINITION FOR AI THINKER MODEL ---
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void sendFrame() {
  camera_fb_t * fb = esp_camera_fb_get();
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  if (centralNodeId != 0) {
    String encoded = base64::encode(fb->buf, fb->len);
    const size_t chunkSize = 10000;
    size_t totalChunks = (encoded.length() + chunkSize - 1) / chunkSize;
    for(size_t i = 0; i < totalChunks; i++) {
      String chunk = "FRAME:" + String(i) + ":" + String(totalChunks) + ":" + encoded.substring(i * chunkSize, min((i + 1) * chunkSize, encoded.length()));
      mesh.sendSingle(centralNodeId, chunk);
      delay(10);
    }
  }
  esp_camera_fb_return(fb);
}

void receivedCallback(uint32_t from, String &msg) {
  if(msg == "CENTRAL") centralNodeId = from;
}
void newConnectionCallback(uint32_t nodeId) {}
void changedConnectionCallback() {}
void nodeTimeAdjustedCallback(int32_t offset) {}

void handleStream() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP32-CAM</title></head><body>";
  html += "<h1>ESP32-CAM Live View</h1>";
  html += "<img id='stream' src='/stream' style='width:100%; max-width:800px;'>";
  html += "<script>setInterval(()=>{document.getElementById('stream').src='/stream?t='+new Date().getTime()},100);</script>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
  Serial.println("=== ESP32-CAM Starting ===");
  delay(1000);
  
  // Setup WiFi Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASS);
  delay(500);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.println("WiFi AP Started");
  Serial.print("AP IP Address: ");
  Serial.println(IP);
  Serial.print("AP MAC: ");
  Serial.println(WiFi.softAPmacAddress());

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 15;
  config.fb_count = 1;

  // Resolusi: SVGA (800x600) atau CIF (400x296) agar stabil
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 12; // 0-63 (makin kecil makin bagus kualitasnya)
  config.fb_count = 1;

  // Init Kamera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera Ready!");
  
  // Setup web server
  server.on("/", handleRoot);
  server.on("/stream", handleStream);
  server.begin();
  Serial.println("Web server started");
  Serial.println("Connect to WiFi: " + String(WIFI_SSID));
  Serial.println("Open browser at: http://192.168.4.1");
}

void loop() {
  server.handleClient();
}

