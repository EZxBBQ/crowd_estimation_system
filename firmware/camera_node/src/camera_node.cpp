#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <WebServer.h>
#include <esp_http_server.h>

// Camera pin configuration for AI-Thinker ESP32-CAM
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

const char* ssid = "esp32cam";
const char* password = "123456789";

WebServer server(80);



void TestPSRAM() 
{
  if (psramFound()) 
  {
    Serial.printf("PSRAM FOUND: %u bytes\n", ESP.getPsramSize());
  } else
  {
    Serial.println("PSRAM NOT FOUND");
  }
}


void InitCamera() 
{
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
  
  // Init with high specs for detection
  if(psramFound())
  {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 12;
    config.fb_count = 2;
  }

  // camera initialization
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");

  // get sensor info
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) 
  {
    Serial.println("Failed to get camera sensor");
    return;
  }
  Serial.printf("Camera sensor detected: PID=0x%02X\n", s->id.PID);
  return;
}


void HandleRoot() 
{
  String html = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{margin:0;font-family:Arial;text-align:center;}";
  html += "img{max-width:100%;height:auto;}</style></head><body>";
  html += "<h1>ESP32-CAM Stream</h1>";
  html += "<img src='/stream' id='stream'>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}


TaskHandle_t CameraCaptureTaskHandle = NULL;
void CameraCapture() 
{
  WiFiClient client = server.client();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected())
  {
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) 
    {
      Serial.println("Camera capture failed");
      return;
    }
    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();
    client.write(fb->buf, fb->len);
    client.println();
    
    esp_camera_fb_return(fb);
    
    if (!client.connected()) break;

    delay(42);
  }
}


void InitWebServer() 
{
  server.on("/", HandleRoot);
  server.on("/stream", CameraCapture);
  server.begin();
  Serial.println("Web server started");
  Serial.print("Camera stream available at: http://");
  Serial.println(WiFi.softAPIP());
}


void setup() 
{
  Serial.begin(115200);
  WiFi.softAP(ssid, password);
  WiFi.setSleep(false);
  delay(2000);

  Serial.println("\n=== ESP32-CAM CAMERA VERIFICATION ===");
  TestPSRAM();
  InitCamera();
  InitWebServer();
  Serial.println("=== VERIFICATION COMPLETE ===\n");
}


void loop() 
{
  server.handleClient();
}
