/*
 * CENTRAL NODE (ESP32 Bridge)
 * Tugas: Terima data Mesh -> Kirim ke Laptop via WiFi.
 */

#include <painlessMesh.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

// --- GANTI INI DENGAN HOTSPOT HP KAMU ---
#define   STATION_SSID     "Pat's"     // <--- GANTI NAMA WIFI
#define   STATION_PASSWORD "00000001"     // <--- GANTI PASS WIFI

// --- KONFIGURASI MESH (Harus sama persis dengan Sensor Node) ---
#define   MESH_PREFIX     "IoT_Crowd_Net"
#define   MESH_PASSWORD   "kelompok26"
#define   MESH_PORT       5555

// --- KONFIGURASI SERVER LAPTOP ---
// IP 10.10.63.39 diambil dari screenshot kamu. 
// TAPI, sebaiknya cek lagi IP Laptop saat sudah connect ke Hotspot HP.
String server_url = "http://10.10.63.39:5000/upload_data"; 

Scheduler userScheduler; 
painlessMesh  mesh;

// Fungsi kirim data ke Laptop
void sendToLaptop(String jsonPayload) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Mulai koneksi
    http.begin(server_url);
    http.addHeader("Content-Type", "application/json");
    
    // Kirim POST Request
    int httpResponseCode = http.POST(jsonPayload);
    
    if(httpResponseCode > 0) {
      Serial.print("Success sending to Laptop. Code: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error sending to Laptop: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Putus! Gagal kirim ke Laptop.");
  }
}

// Callback saat terima data dari Mesh (Sensor/Kamera)
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  
  // Teruskan langsung pesan JSON dari Mesh ke Laptop
  sendToLaptop(msg);
}

void setup() {
  Serial.begin(115200);

  // Setup Mesh
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION ); 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);

  // Konfigurasi Bridge (Root)
  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
}

void loop() {
  mesh.update();
}