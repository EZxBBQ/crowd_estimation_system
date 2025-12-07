# IoT Crowd Estimation System

Proyek ini adalah sistem penghitung jumlah orang otomatis di dalam ruangan.
Sistem ini menggabungkan dua metode:
1. **Sensor IR:** Menghitung jumlah orang keluar-masuk (Real Count).
2. **Kamera (AI):** Memotret ruangan untuk mengetahui seberapa padat isinya menggunakan kecerdasan buatan (CSRNet).

Data dari semua alat dikirim tanpa kabel (Mesh Network) ke satu Laptop Server untuk ditampilkan di Website.

---

## ⚙️ Cara Kerja Sistem

1. **Node Sensor:** Mendeteksi orang lewat (masuk/keluar) dan menampilkan jumlahnya di layar LCD kecil.
2. **Node Kamera:** Mengambil foto ruangan setiap beberapa detik.
3. **Central Node:** Menerima data dari Sensor dan Kamera, lalu meneruskannya ke Laptop.
4. **Laptop Server:** Memproses foto menggunakan AI (untuk tahu kepadatan) dan menampilkan Dashboard Website.

---

## 🛠️ Daftar Alat (Hardware)

* **1x ESP32-CAM:** Untuk mengambil gambar.
* **2x ESP32 Biasa (DevKit):** * 1 buah untuk Central Node (Penerima).
    * 1 buah untuk Sensor Node.
* **2x Sensor IR (E18-D80NK):** Untuk menghitung arah (Masuk & Keluar).
* **1x LCD 16x2 (dengan I2C):** Untuk menampilkan jumlah orang di tempat.
* **1x Power Supply (5V 10A):** Sumber listrik untuk semua alat.

---

## 🔌 Panduan Kabel Singkat

1.  **Power Supply:**
    * Hubungkan kabel `V+` dan `V-` dari Power Supply ke pin `5V` dan `GND` di **semua** ESP32.
    * *Penting:* Gunakan kabel yang agak tebal agar listrik stabil.
2.  **Node Sensor:**
    * Sensor IR Masuk -> Pin GPIO (cek codingan).
    * Sensor IR Keluar -> Pin GPIO (cek codingan).
    * LCD -> Pin SDA (21) dan SCL (22).

---

## 🚀 Cara Instalasi & Menjalankan

### Tahap 1: Setup Server (Laptop)
1.  Clone repository ini.
2.  Install dependencies Python:
    ```bash
    pip install flask torch torchvision opencv-python
    ```
3.  Jalankan server:
    ```bash
    cd backend
    python app.py
    ```
4.  **Catat IP Address Laptop** (Contoh: `192.168.1.10`). Pastikan Laptop dan Central Node terhubung ke Wi-Fi yang sama (atau Hotspot HP).

### Tahap 2: Flash Firmware (ESP32)
1.  Buka folder `firmware` di Arduino IDE.
2.  **Central Node:**
    * Buka `central_node.ino`.
    * Ubah `String server_url = "http://192.168.1.10:5000/upload";` sesuai IP Laptop.
    * Upload ke ESP32 Central.
3.  **Camera Node:**
    * Upload `camera_node.ino` ke ESP32-CAM.
4.  **Sensor Node:**
    * Upload `sensor_node.ino` ke ESP32 Sensor.

### Tahap 3: Operasional
1.  Nyalakan Power Supply.
2.  Tunggu sekitar 10-30 detik hingga jaringan Mesh terbentuk (LED indikator biasanya berkedip).
3.  Pantau hasil di Dashboard: `http://localhost:5000`.

---
