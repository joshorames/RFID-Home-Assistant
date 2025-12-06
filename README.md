# ESP32 RFID Home Assistant

Turn an ESP32-S3 (XIAO) and a PN532 NFC reader into a **smart doorbell system**:

- Configure Wi-Fi from a **captive-style web portal**
- Scan RFID/NFC tags with a **PN532**
- Store tags in **flash memory (Preferences)**
- Assign a **custom Alexa / HTTP URL per tag**
- Trigger different **Alexa routines / webhooks** based on which tag was scanned
- Manage everything from a simple **web UI**

---

## ✨ Features

- 🔐 **NFC / RFID Authentication**
  - Uses **PN532** in I²C + IRQ mode
  - Reads card UID and uses it as the key

- 🌐 **Wi-Fi Setup Portal**
  - First run: starts an **AP** named `ESP32-Setup`
  - Enter SSID/password via browser
  - On boot: tries saved Wi-Fi, falls back to **AP+STA** if connect fails

- 📡 **Per-Tag Custom Doorbell URLs**
  - Each RFID tag can have its own **custom URL**
  - Perfect for **Virtual Smart Home**, **IFTTT**, **Home Assistant**, etc.
  - New tags default to a **global URL**, but can be edited

- 🖥️ **Built-in Web UI**
  - View Wi-Fi status
  - Edit Wi-Fi credentials
  - Edit global default doorbell URL
  - Add new RFID tags
  - View all stored tags in a **table**
  - Edit the URL for each tag
  - Delete tags
  - **Test Doorbell** button (manual trigger)

- 💾 **Persistent Storage**
  - Uses `Preferences` to store:
    - Wi-Fi credentials
    - Global default URL
    - List of tag UIDs
    - URL per tag

---

## 🧱 Hardware

- **Microcontroller**
  - Seeed Studio **XIAO ESP32-S3**

- **NFC Reader**
  - PN532 NFC module (I²C mode, with IRQ pin)

---

## 🔌 Wiring

> These are the pins used in the code. Adjust only if you also update the pin definitions in the sketch.

### PN532 → XIAO ESP32-S3

| PN532 Pin | XIAO Pin | Notes                    |
|-----------|----------|--------------------------|
| SDA       | D6       | I²C data                 |
| SCL       | D5       | I²C clock                |
| IRQ       | D3       | Interrupt from PN532     |
| RST / RSTO| D6       | Reset (matches your code)|
| VCC       | 3.3V/5V  | Depends on PN532 board   |
| GND       | GND      | Common ground            |

> ⚠️ Make sure your PN532 is set to **I²C mode** (check solder jumpers or DIP switch on your module).

---

## 📦 Software Overview

The sketch combines:

- `WiFi.h`, `WebServer.h`, `Preferences.h`, `HTTPClient.h`
- `Adafruit_PN532.h` for PN532 NFC
- Simple HTML interface served from the ESP32

### Namespaces in Preferences

- `wifi`
  - `ssid` – stored Wi-Fi SSID
  - `password` – stored Wi-Fi password

- `config`
  - `doorbellURL` – global default doorbell URL

- `rfid`
  - `count` – number of stored tags
  - `tagN` – UID string for tag `N`
  - `urlN` – doorbell URL for tag `N`
  - `waiting` – flag: next scanned tag should be added

---

## 🚀 Getting Started

### 1. Flash the Firmware

1. Open the `.ino` file in **Arduino IDE** (or PlatformIO).
2. Select:
   - Board: **Seeed XIAO ESP32S3** (or equivalent)
   - Correct COM port.
3. Install required libraries:
   - `Adafruit PN532`
   - `ESP32` board support (via Boards Manager)
4. Upload the sketch.

---

### 2. First Boot: Wi-Fi Setup (AP Mode)

On first boot (no saved Wi-Fi):

1. The ESP32 will start an AP:
   - **SSID:** `ESP32-Setup`
   - **Password:** `12345678`
2. Connect your phone/laptop to this Wi-Fi.
3. Open a browser and go to:  
   `http://192.168.4.1/` (typical AP IP)
4. You’ll see the **web portal**:
   - Enter **SSID** and **Password**
   - Click **Save WiFi**

The device will reboot into **STA mode**, try to connect, and stay in STA or AP+STA depending on success.

---

### 3. Web Interface Overview

Once on your normal Wi-Fi:

1. Find the ESP32’s IP (via Serial Monitor or router).
2. Go to `http://<ESP_IP>/` in your browser.

You’ll see sections:

#### 🔹 Wi-Fi Status
Shows whether the device is **Connected** or **Not Connected**.

#### 🔹 Default Doorbell URL

A form like:

```text
Default Doorbell URL
[ https://www.virtualsmarthome.xyz/url_routine_trigger/... ]
[ Save Default URL ]
