/*
 * CENTRAL NODE (ESP32 Bridge)
 * Tugas: Terima data Mesh -> Kirim ke Laptop via WiFi.
 */

#include <painlessMesh.h>

// --- KONFIGURASI MESH (Harus sama persis dengan node lain) ---
#define   MESH_PREFIX     "CrowdMesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Callback saat terima data dari Mesh (Sensor/Kamera)
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  // Proses data di sini (misal: simpan, tampilkan, dsb)
}

void setup() {
  Serial.begin(115200);
  // Setup Mesh
  mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
}

void loop() {
  mesh.update();
}
