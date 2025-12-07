# ESP32 RFID Home Assistant

<img width="945" height="438" alt="image" src="https://github.com/user-attachments/assets/a99cedb7-bd15-4366-9cb8-0a6503e1f76b" />


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
```

This URL is used for:

The “Test Doorbell” button

Newly added tags (initial URL value)

As a fallback if a tag doesn’t have a custom URL saved

🔹 Wi-Fi Configuration

You can update saved SSID and password here at any time:

SSID:     [ your-ssid ]
Password: [ ******** ]
[ Save WiFi ]

🔹 Doorbell Test
[Test Doorbell]


This sends an HTTP GET to the currently saved default doorbell URL.

4. Adding a New RFID/NFC Tag

Go to the web UI.

Click “Add Next RFID Scan”.

Within a few seconds, present your NFC card/tag to the PN532.

Serial output will show something like:

Card UID: 04AABBCCDD
Saved new tag: 04AABBCCDD


Reload the main page — you’ll see a new row in the Stored RFID Tags table.

5. Editing Tag-Specific URLs

In the Stored RFID Tags table, each row looks like:

UID	Doorbell URL	Actions
04AABBCCDD	https://your-url-for-this-tag.com/...	[Save] [delete]

You can:

Change the URL text

Click Save to update that tag’s URL

Click [delete] to remove the tag completely

On scan:

If the UID is found in the list:

The matching URL for that UID is triggered.

If the UID is not in the list:

It’s treated as unknown (no URL call).

🔔 How the Doorbell Trigger Works

The code uses simple HTTP GET requests:

HTTPClient http;
http.begin(urlOverride);           // or doorbellURL if using default
int code = http.GET();
http.end();


This works perfectly with:

Virtual Smart Home

IFTTT Webhooks

Home Assistant automations

Any service that reacts to an HTTP GET URL

Each RFID tag can trigger a different webhook, so you can:

One tag for front door

Another tag for garage

Another for package dropoff

Etc.

🔁 Boot Flow & Wi-Fi Logic

On startup:

Load ssid and password from Preferences.

If no Wi-Fi saved:

Start AP only for configuration.

If Wi-Fi saved:

Try to connect as STA for ~10 seconds.

If connect fails:

Enable AP+STA:

Still tries STA.

At the same time hosts ESP32-Setup for reconfiguration.

PN532 initialization happens after Wi-Fi setup, and if the reader isn’t found, the device will halt with:

Didn't find PN53x board

 

📄 License

Add your preferred license here, for example:

MIT License


Or keep it private for personal use.

📚 Credits

ESP32-S3 (XIAO) by Seeed Studio

PN532 support via Adafruit PN532 Library

Virtual Smart Home / Alexa URL integration via customizable HTTP endpoints
