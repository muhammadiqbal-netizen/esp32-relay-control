# Setup Guide - ESP32 Relay Control System

## 📋 Prerequisites

- ESP32 DevKit board
- 4 Channel Relay Module (5V)
- Power Supply (5V / 2A)
- USB Cable untuk upload
- Arduino IDE
- WiFi connection

## 🚀 Step-by-Step Setup

### Step 1: Upload ESP32 Firmware

#### 1.1 Install Arduino IDE
- Download dari: https://www.arduino.cc/en/software

#### 1.2 Install ESP32 Board Support
1. Buka Arduino IDE
2. **File → Preferences**
3. Di bagian "Additional Boards Manager URLs", tambahkan:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
4. **Tools → Board → Board Manager**
5. Search "esp32" dan install "esp32 by Espressif Systems"

#### 1.3 Install Required Libraries
1. **Sketch → Include Library → Manage Libraries**
2. Cari dan install:
   - `ArduinoJson` (by Benoit Blanchon)

#### 1.4 Configure Settings
1. Buka `esp32/main.ino`
2. Edit WiFi credentials:
   ```cpp
   #define SSID "your_wifi_ssid"
   #define PASSWORD "your_wifi_password"
   ```

#### 1.5 Select Board & Upload
1. **Tools → Board → ESP32 Dev Module**
2. **Tools → Port → COM#** (pilih port ESP32)
3. Klik **Upload**
4. Tunggu hingga selesai (±30 detik)

#### 1.6 Verify Upload
1. Buka **Tools → Serial Monitor**
2. Set baud rate ke 115200
3. Restart ESP32 (tekan reset button)
4. Seharusnya muncul info IP address

### Step 2: Wiring Relay Module

```
ESP32           Relay Module      Beban AC/DC
─────────────────────────────────────────────
GPIO 16 (D16) → IN1
GPIO 17 (D17) → IN2
GPIO 18 (D18) → IN3
GPIO 19 (D19) → IN4
GND        → GND

              COM1 → Beban 1
              NO1  → Live AC
              NC1  → (tidak digunakan)
```

**Catatan:** COM, NO, NC adalah terminal relay. 
- COM = Common (input)
- NO = Normally Open (output)
- NC = Normally Closed (tidak perlu)

### Step 3: Setup Web Interface

#### 3.1 Untuk Development
1. Buka file `web/index.html` di browser
2. Atau gunakan Live Server (VS Code extension)

#### 3.2 Untuk Production
Deploy ke web server:
```bash
# Copy folder web ke web server
# atau gunakan hosting seperti Netlify, Vercel, GitHub Pages
```

### Step 4: Setup Backend (Optional)

#### 4.1 Install Node.js
- Download dari: https://nodejs.org/ (LTS version)

#### 4.2 Setup Backend
```bash
cd backend
npm install
```

#### 4.3 Configure Backend
Buat file `.env`:
```
PORT=3000
ESP32_HOST=192.168.1.100    # Ganti dengan IP ESP32 Anda
ESP32_PORT=8080
NODE_ENV=development
CORS_ORIGIN=*
```

#### 4.4 Run Backend
```bash
npm start
```

Server akan berjalan di `http://localhost:3000`

### Step 5: Configure Web Interface

1. Buka `web/index.html`
2. Klik tombol ⚙️ **Settings**
3. Masukkan IP Address ESP32
4. Set port ke 8080
5. Klik **Simpan**
6. Seharusnya status berubah menjadi "Online"

## 🔧 Troubleshooting

### Problem: ESP32 tidak terdeteksi di Arduino IDE
**Solution:**
- Install USB driver CH340 (biasanya sudah built-in di Windows 10/11)
- Atau install dari: https://sparks.gogo.co.nz/ch340.html

### Problem: Upload gagal "Failed to connect to ESP32"
**Solution:**
- Tekan dan tahan tombol BOOT pada ESP32
- Klik Upload
- Lepaskan tombol BOOT setelah upload dimulai

### Problem: Web interface tidak terhubung ke ESP32
**Solution:**
1. Pastikan ESP32 sudah power on
2. Check IP address ESP32 (lihat serial monitor)
3. Pastikan firewall tidak memblokir port 8080
4. Coba ping IP ESP32 dari terminal:
   ```bash
   ping 192.168.1.100
   ```

### Problem: Relay tidak merespons
**Solution:**
1. Check wiring ke GPIO pins
2. Verifikasi power supply cukup
3. Lihat serial monitor untuk error messages
4. Test relay manual dengan multimeter

### Problem: "Timeout" saat toggle relay
**Solution:**
- Naikkan timeout di `web/config.js`
- Cek kecepatan WiFi
- Pastikan tidak ada interference WiFi

## 📊 Testing

### Test 1: Check ESP32 Web Server
```bash
curl http://192.168.1.100:8080/status
```

### Test 2: Toggle Relay 1
```bash
curl -X POST http://192.168.1.100:8080/relay/1 \
  -H "Content-Type: application/json" \
  -d '{"state": true}'
```

### Test 3: Get All Relay Status
```bash
curl http://192.168.1.100:8080/relay
```

## 📱 Akses dari Smartphone

1. Pastikan smartphone dan ESP32 dalam 1 WiFi network
2. Catat IP address ESP32
3. Akses dari browser smartphone: `http://<IP_ESP32>/`

## 🌐 Akses dari Internet (Cloud)

Untuk akses ESP32 dari internet:

### Option 1: Static IP & Port Forwarding
1. Setup static IP di router
2. Port forward port 8080
3. Catat public IP
4. Akses dari: `http://public_ip:8080`

⚠️ **Risky!** Tidak recommended tanpa security

### Option 2: VPN
Gunakan VPN untuk akses aman

### Option 3: Cloud Platform
1. Deploy backend ke cloud (Heroku, Railway, Render)
2. Backend forward ke ESP32 via LAN
3. Akses via cloud API

## 📈 Advanced Configuration

### 1. Custom GPIO Pins
Edit di `esp32/main.ino`:
```cpp
#define RELAY1_PIN 16
#define RELAY2_PIN 17
#define RELAY3_PIN 18
#define RELAY4_PIN 19
```

### 2. Custom Relay Names
Edit di `esp32/main.ino`:
```cpp
const char* relayNames[4] = {
    "Nama Relay 1",
    "Nama Relay 2",
    "Nama Relay 3",
    "Nama Relay 4"
};
```

### 3. Auto Save State
ESP32 menyimpan state ke EEPROM secara otomatis.
Relay akan tetap ON/OFF bahkan setelah restart.

## 🔐 Security Recommendations

Untuk production use:

1. **Enable Authentication**
   - Tambahkan password/token
   - Implement JWT

2. **Use HTTPS**
   - Self-signed SSL certificate
   - Let's Encrypt

3. **Firewall Rules**
   - Restrict port access
   - IP whitelist

4. **Rate Limiting**
   - Limit requests per minute
   - Prevent DOS attack

5. **Monitoring**
   - Log semua activity
   - Alert on anomaly

## 📞 Support

Jika ada masalah:
1. Check Serial Monitor output
2. Verify network connectivity
3. Test API dengan curl/Postman
4. Check browser console for errors

---

**Happy Tinkering! 🎉**
