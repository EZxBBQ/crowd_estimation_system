/*
 * SENSOR NODE (Final Version with Sequence Logic)
 * Hardware: ESP32 DevKit V1 + 2x IR Sensor + LCD I2C
 * * LOGIKA PEMASANGAN:
 * [Luar Ruangan]  -->  [Sensor 1 (Pin 18)]  -->  [Sensor 2 (Pin 19)]  -->  [Dalam Ruangan]
 */

#include <painlessMesh.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <Wire.h>

// ==========================================
// 1. KONFIGURASI JARINGAN MESH
// ==========================================
#define   MESH_PREFIX     "CrowdMesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555

// ==========================================
// 2. KONFIGURASI PIN (SESUAIKAN DISINI)
// ==========================================
// Sensor 1: Posisi di LUAR pintu (Awal masuk)
#define PIN_SENSOR_1    25  

// Sensor 2: Posisi di DALAM pintu (Akhir masuk)
#define PIN_SENSOR_2    27

#define SDA_PIN 21
#define SCL_PIN 22  

// Alamat I2C LCD (Biasanya 0x27, kalau gagal coba 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ==========================================
// 3. VARIABEL GLOBAL
// ==========================================
Scheduler userScheduler; 
painlessMesh  mesh;

int peopleCount = 0;
String crowdLevel = "LOW"; // Default crowd level

// Variabel Logika Urutan (State Machine)
// 0 = Diam/Standby
// 1 = Proses MASUK dimulai (Sensor 1 kena duluan)
// 2 = Proses KELUAR dimulai (Sensor 2 kena duluan)
int sequenceState = 0; 

unsigned long sequenceTimer = 0;
const int timeout = 2500; // Reset otomatis jika diam lebih dari 2.5 detik

// ==========================================
// 4. FUNGSI-FUNGSI PENDUKUNG
// ==========================================

// Fungsi Update Tampilan LCD
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Count:");
  lcd.print(peopleCount);
  lcd.print(" ");
  
  // Status kecil di pojok kanan atas
  lcd.setCursor(13, 0);
  if (sequenceState == 1) lcd.print("->"); 
  else if (sequenceState == 2) lcd.print("<-"); 
  else lcd.print("OK");
  
  // Tampilkan crowd level di baris kedua
  lcd.setCursor(0, 1);
  lcd.print("Level: ");
  lcd.print(crowdLevel);
}

void sendDataToMesh() {
  JsonDocument doc;
  doc["node"] = "sensor";
  doc["count"] = peopleCount;
  String msg;
  serializeJson(doc, msg);
  Serial.printf("[SENSOR] Sending count=%d\n", peopleCount);
  mesh.sendBroadcast(msg);
}

// Fungsi Terima Data dari Central Node (Mesh)
void receivedCallback(uint32_t from, String &msg) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, msg);
  
  if (!error && doc.containsKey("node") && doc["node"] == "central") {
    // Terima crowd level dari central node
    if (doc.containsKey("crowdLevel")) {
      crowdLevel = doc["crowdLevel"].as<String>();
      Serial.print("✅ Crowd Level Updated: "); Serial.println(crowdLevel);
      updateLCD();
    }
  }
}

// ==========================================
// 5. LOGIKA UTAMA (PEMBACAAN SENSOR)
// ==========================================
void readSensors() {
  // Baca status sensor (LOW = Ada Orang, HIGH = Kosong)
  int s1 = digitalRead(PIN_SENSOR_1);
  int s2 = digitalRead(PIN_SENSOR_2);

  // --- A. CEK TIMEOUT (RESET) ---
  // Jika seseorang memotong sensor 1 tapi tidak lanjut ke sensor 2 dalam waktu lama
  if (sequenceState != 0 && (millis() - sequenceTimer > timeout)) {
    sequenceState = 0; // Reset ke posisi diam
    Serial.println("⚠️ Timeout! Orang tidak jadi lewat. Reset urutan.");
    updateLCD();
  }

  // --- B. DETEKSI AWAL (START) ---
  if (sequenceState == 0) {
    // Jika Sensor 1 kena duluan (dan Sensor 2 bersih) -> OTW MASUK
    if (s1 == LOW && s2 == HIGH) {
      sequenceState = 1; 
      sequenceTimer = millis();
      Serial.println("➡️ Proses MASUK dimulai (Sensor 1 Kena)...");
      updateLCD();
    } 
    // Jika Sensor 2 kena duluan (dan Sensor 1 bersih) -> OTW KELUAR
    else if (s2 == LOW && s1 == HIGH) {
      sequenceState = 2;
      sequenceTimer = millis();
      Serial.println("⬅️ Proses KELUAR dimulai (Sensor 2 Kena)...");
      updateLCD();
    }
  }

  // --- C. DETEKSI AKHIR (FINISH) ---
  
  // LOGIKA MASUK: Tadi statusnya 1, sekarang Sensor 2 kena juga
  if (sequenceState == 1 && s2 == LOW) {
    peopleCount++;
    Serial.println("✅ SUKSES: Orang Masuk (+1)");
    
    // Kirim & Update
    updateLCD();
    sendDataToMesh();
    
    // Reset & Delay biar tidak double count
    sequenceState = 0; 
    delay(500); 
  }
  
  // LOGIKA KELUAR: Tadi statusnya 2, sekarang Sensor 1 kena juga
  else if (sequenceState == 2 && s1 == LOW) {
    if (peopleCount > 0) peopleCount--; // Jangan sampai minus
    Serial.println("🔻 SUKSES: Orang Keluar (-1)");
    
    // Kirim & Update
    updateLCD();
    sendDataToMesh();
    
    // Reset & Delay
    sequenceState = 0; 
    delay(500); 
  }
}

// Task Scheduler: Jalankan fungsi sensor setiap 50 milidetik
Task taskReadSensors(50, TASK_FOREVER, &readSensors);
Task taskUpdateLCD(1000, TASK_FOREVER, &updateLCD);

// ==========================================
// 6. SETUP & LOOP
// ==========================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Scanning I2C...");
  bool i2cFound = false;
  for(byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if(Wire.endTransmission() == 0) {
      Serial.printf("I2C device found at 0x%02X\n", i);
      i2cFound = true;
    }
  }
  if(!i2cFound) Serial.println("No I2C devices found!");

  pinMode(PIN_SENSOR_1, INPUT_PULLUP); 
  pinMode(PIN_SENSOR_2, INPUT_PULLUP);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Crowd Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Count: 0");
  Serial.println("LCD Initialized");
  delay(2000);

  // Setup Mesh Network
  // Debug Message: ERROR dan STARTUP saja biar serial monitor gak penuh spam
  mesh.setDebugMsgTypes( ERROR | STARTUP ); 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback); // Set callback untuk terima data dari mesh

  // Aktifkan Task Sensor
  userScheduler.addTask( taskReadSensors );
  taskReadSensors.enable();
  
  userScheduler.addTask( taskUpdateLCD );
  taskUpdateLCD.enable();
  
  Serial.println("=============================================");
  Serial.println("   SISTEM SENSOR URUTAN (SEQUENCE) AKTIF");
  Serial.println("=============================================");
  Serial.println("Siap mendeteksi gerakan...");
}

void loop() {
  // Mesh butuh jalan terus di background
  mesh.update();
}