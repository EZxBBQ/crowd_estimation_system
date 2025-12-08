#include <Arduino.h>
#include "esp_camera.h"
#include <painlessMesh.h>
#include <base64.h>

#define   MESH_PREFIX     "CrowdMesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555
#define   FRAME_DELAY     100
#define   CAMERA_MODEL_AI_THINKER 

Scheduler userScheduler;
painlessMesh mesh;
uint32_t centralNodeId = 0;
unsigned long lastTime = 0;

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


void setup() {
  Serial.begin(115200);
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

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
}

void loop() {
  mesh.update();
  if ((millis() - lastTime) > FRAME_DELAY) {
    sendFrame();
    lastTime = millis();
  }
}

