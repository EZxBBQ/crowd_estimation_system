#include "esp_camera.h"
#include <painlessMesh.h>


// --- KONFIGURASI MESH ---
#define   MESH_PREFIX     "CrowdMesh"   // Nama mesh
#define   MESH_PASSWORD   "meshpassword" // Password mesh
#define   MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;
uint32_t centralNodeId = 0; // Set di central node

// --- PENGATURAN WAKTU ---
unsigned long timerDelay = 10000; // Ambil gambar tiap 10 detik
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

void setup() {
  Serial.begin(115200);


  // 1. Inisialisasi Mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);  // Debug
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // 2. Konfigurasi Kamera
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

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
  // Cek timer
  if ((millis() - lastTime) > timerDelay) {
    sendImage();
    lastTime = millis();
  }
}

void sendImage() {
  camera_fb_t * fb = NULL;
  // Ambil Gambar
  fb = esp_camera_fb_get(); 
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  // Kirim notifikasi ke central node via mesh (bukan gambar langsung)
  if (centralNodeId != 0) {
    mesh.sendSingle(centralNodeId, "Image captured");
    Serial.println("Image notification sent to central node via mesh!");
  } else {
    Serial.println("Central node ID not set!");
  }
  esp_camera_fb_return(fb);
}

// Callback mesh
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
  // Set centralNodeId jika nodeId adalah central node
  // (Bisa dengan pesan khusus dari central node)
}
void changedConnectionCallback() {
  Serial.println("Changed connections");
}
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time adjusted by %d\n", offset);
}
