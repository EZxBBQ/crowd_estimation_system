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

## 🚀 Cara Menjalankan

### 1. Siapkan Server (Laptop)
Pastikan Python sudah terinstall, lalu install library yang dibutuhkan:
```bash
pip install flask torch torchvision opencv-python
