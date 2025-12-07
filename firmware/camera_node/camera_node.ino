#include "esp_camera.h"
#include <WiFi.h>

// --- KONFIGURASI WIFI ---
const char* ssid = "Pat's";        // <--- GANTI INI
const char* password = "00000001";    // <--- GANTI INI

// --- KONFIGURASI SERVER ---
// Masukkan IP Laptop di sini (TANPA "http://")
const char* serverIp = "10.10.63.39";      // <--- GANTI IP LAPTOP (Cek ipconfig)
const int serverPort = 5000;               // Port Flask (Default 5000)

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

  // 1. Konek WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

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
  // Cek timer
  if ((millis() - lastTime) > timerDelay) {
    if(WiFi.status() == WL_CONNECTED){
      sendImage();
    } else {
      Serial.println("WiFi Disconnected, reconnecting...");
      WiFi.reconnect();
    }
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

  // Gunakan WiFiClient untuk kontrol penuh TCP
  WiFiClient client;
  
  Serial.println("Connecting to server...");
  if (client.connect(serverIp, serverPort)) {
    Serial.println("Connected! Sending image...");
    
    // Persiapan Multipart Body
    String boundary = "MyBoundary";
    String head = "--" + boundary + "\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"capture.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--" + boundary + "--\r\n";

    uint32_t imageLen = fb->len;
    uint32_t extraLen = head.length() + tail.length();
    uint32_t totalLen = imageLen + extraLen;

    // Kirim HTTP Headers manual
    client.println("POST /upload_image HTTP/1.1");
    client.println("Host: " + String(serverIp));
    client.println("Content-Length: " + String(totalLen));
    client.println("Content-Type: multipart/form-data; boundary=" + boundary);
    client.println(); // Baris kosong pemisah header dan body

    // Kirim Body (Head -> Image -> Tail)
    client.print(head);
    
    // Kirim gambar per chunk (potongan) 1KB agar RAM aman
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    
    for (size_t n=0; n<fbLen; n=n+1024) {
      if (n+1024 < fbLen) {
        client.write(fbBuf, 1024);
        fbBuf += 1024;
      } else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client.write(fbBuf, remainder);
      }
    }   
    
    client.print(tail);
    
    // Baca Respons Server
    int timeout = 0;
    while(client.connected() && timeout < 5000) {
      if(client.available()) {
        String line = client.readStringUntil('\n');
        Serial.println(line); // Print respons server
        break; 
      }
      delay(1);
      timeout++;
    }
    
    client.stop();
    Serial.println("Image Sent Successfully!");
  } else {
    Serial.println("Connection to Server Failed.");
  }
  
  // Wajib kembalikan memori kamera
  esp_camera_fb_return(fb); 
}