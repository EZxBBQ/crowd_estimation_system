/*
 * SENSOR NODE (ESP32 + IR + LCD)
 * Tugas: Baca arah IR (Masuk/Keluar), Update LCD, Kirim ke Mesh.
 */

#include <Arduino.h>
#include <painlessMesh.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>

// --- KONFIGURASI MESH ---
#define   MESH_PREFIX     "CrowdMesh"
#define   MESH_PASSWORD   "meshpassword"
#define   MESH_PORT       5555

// --- KONFIGURASI PIN ---
#define IR_IN_PIN   18  // Sensor Masuk (GPIO 18)
#define IR_OUT_PIN  19  // Sensor Keluar (GPIO 19)

// Alamat LCD biasanya 0x27. Jika tidak muncul, coba 0x3F.
LiquidCrystal_I2C lcd(0x27, 16, 2);

Scheduler userScheduler; 
painlessMesh  mesh;

// Variabel Data
int peopleCount = 0;
bool lastInState = HIGH;
bool lastOutState = HIGH;

// --- FUNGSI UPDATE LCD ---
void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Crowd Monitor");
  lcd.setCursor(0, 1);
  lcd.print("People: ");
  lcd.print(peopleCount);
}

// --- FUNGSI KIRIM DATA KE MESH ---
void sendDataToMesh() {
  DynamicJsonDocument doc(1024);
  doc["node"] = "sensor";
  doc["count"] = peopleCount;

  String msg;
  serializeJson(doc, msg);
  
  // Kirim Broadcast agar Central Node mendengar
  mesh.sendBroadcast(msg);
  Serial.println("Sent to Mesh: " + msg);
}

// --- FUNGSI BACA SENSOR ---
void readSensors() {
  int inState = digitalRead(IR_IN_PIN);
  int outState = digitalRead(IR_OUT_PIN);

  // Logika Sederhana (Active LOW = Ada Orang)
  // Cek Orang Masuk
  if (lastInState == HIGH && inState == LOW) {
    peopleCount++;
    Serial.println("Orang Masuk!");
    updateLCD();
    sendDataToMesh();
    delay(250); // Delay dikit biar ga double count
  }

  // Cek Orang Keluar
  if (lastOutState == HIGH && outState == LOW) {
    if (peopleCount > 0) peopleCount--;
    Serial.println("Orang Keluar!");
    updateLCD();
    sendDataToMesh();
    delay(250);
  }

  lastInState = inState;
  lastOutState = outState;
}

// Task untuk membaca sensor terus menerus
Task taskReadSensors(50, TASK_FOREVER, &readSensors);

void setup() {
  Serial.begin(115200);

  // Setup Pin
  pinMode(IR_IN_PIN, INPUT); // Gunakan INPUT_PULLUP jika sensor butuh
  pinMode(IR_OUT_PIN, INPUT);

  // Setup LCD
  lcd.init();
  lcd.backlight();
  updateLCD();

  // Setup Mesh
  mesh.setDebugMsgTypes( ERROR | STARTUP ); 
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );

  userScheduler.addTask( taskReadSensors );
  taskReadSensors.enable();
}

void loop() {
  mesh.update();
}
