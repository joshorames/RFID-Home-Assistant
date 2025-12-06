Turn NFC tags into smart doorbells.
This project uses an ESP32, a PN532 NFC reader, and a small web control panel to:

Configure Wi-Fi from your phone (no hardcoded SSID/password)

Add NFC tags via a button in the web UI

Assign each NFC tag its own custom doorbell URL (e.g. Alexa Virtual Smart Home routines)

Edit or delete tags from a table in the browser

Trigger HTTP doorbell actions automatically when a known tag is scanned

No mobile app. No flashing screens. Just tap a tag → run an automation.

✨ Features

Wi-Fi setup portal

First boot: ESP32 starts in Access Point mode as ESP32-Setup

Configure Wi-Fi SSID / password from your phone or laptop

Wi-Fi credentials are stored in flash (Preferences), so you don’t have to re-enter them on every reboot

Dual-mode Wi-Fi after setup

If saved Wi-Fi connects → normal STA mode

If connection fails → STA + AP mode: keeps trying Wi-Fi while still exposing the config portal (ESP32-Setup)

NFC / RFID tag reader

Uses PN532 in IRQ/I²C mode

Reads card UID and displays it over Serial

Debounces reads so you don’t trigger multiple actions from one tap

Web control panel

Shows Wi-Fi status

Lets you:

Edit SSID / password

Edit default doorbell URL

Add a new tag (press button, then scan tag)

See a table of stored tags:

UID

per-tag doorbell URL

Save / Delete actions

Per-tag doorbell actions

Each UID can have its own URL

New tags default to the global doorbell URL (which you can edit)

On scan:

If tag is known → its URL is called via HTTP GET

If tag is unknown → ignored (just logged to Serial)

Alexa / Virtual Smart Home compatible

Designed to work with Virtual Smart Home URL routines

But any HTTP GET endpoint works (Home Assistant, Node-RED, IFTTT, etc.)

🧩 Hardware

Minimum setup:

1× ESP32 board

Code is written / tested for Seeed XIAO ESP32-S3, but will work on other ESP32s with pin adjustment.

1× PN532 NFC module (I²C/IRQ mode)

USB cable for programming

NFC tags / cards (e.g. Mifare Classic)

🔌 Pin connections (XIAO ESP32-S3 + PN532, current code)

Make sure your PN532 is configured for I²C mode (check jumpers / solder pads).

PN532 Pin	XIAO ESP32-S3 Pin	Notes
SDA	D6	I²C data
SCL	D5	I²C clock
IRQ	D3	Interrupt line to ESP32
RST / RSTO	D6 (or as per your board)	Must match code / wiring
VCC	3.3V or 5V	Depends on PN532 module
GND	GND	Common ground

⚠️ Important:
The code assumes:

#define PN532_IRQ   (D3)
#define PN532_RESET (D6)


and relies on your board core’s I²C mapping (SDA=D6, SCL=D5). If you change wiring or board, update pins accordingly.

🛠️ Software Setup
1. Arduino IDE & Board Support

Install Arduino IDE (or PlatformIO if you prefer).

Install the ESP32 board package via Boards Manager.

Select your board (e.g. Seeed XIAO ESP32S3).

2. Libraries

Make sure these libraries are installed (via Library Manager):

Adafruit PN532

Adafruit BusIO (dependency of PN532)

ESP32 core includes:

WiFi.h

WebServer.h

Preferences.h

HTTPClient.h

The sketch already includes:

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>


No other external dependencies required.

🚀 Getting Started
1. Flash the Firmware

Open the .ino file in Arduino IDE.

Select the correct board + port.

Upload the sketch.

Open Serial Monitor at 115200 baud to watch logs.

2. First Boot – Connect to ESP32 Setup Portal

If there are no saved Wi-Fi credentials, the ESP32 will start as:

SSID: ESP32-Setup

Password: 12345678

Steps:

On your phone/PC, connect to Wi-Fi network ESP32-Setup.

Open a browser and go to http://192.168.4.1/ (typical AP IP).

You’ll see the ESP32 NFC Doorbell page.

3. Configure Wi-Fi & Default Doorbell URL

On the web page:

Wi-Fi Configuration

Enter your home router SSID & password.

Click “Save WiFi”.

The ESP32 will reboot into STA mode and attempt to connect.

Default Doorbell URL

At the top of the page is “Default Doorbell URL”.

Paste your Virtual Smart Home / Alexa URL or any HTTP GET endpoint.

Click “Save Default URL”.

🔁 After Wi-Fi is saved:

On future reboots, the ESP32 will try to connect using the saved SSID/password.

If it cannot connect within ~10 seconds, it switches to AP + STA:

It keeps trying Wi-Fi.

It also exposes the ESP32-Setup AP so you can fix creds.

4. Adding RFID Tags

Go to the web UI (either via AP IP or your router-assigned IP).

Click “Add Next RFID Scan”.

Now tap an NFC tag/card on the PN532.

In Serial Monitor you’ll see something like:

Card UID: 04A1B2C3D4
Saved new tag: 04A1B2C3D4


The tag will now appear in the “Stored RFID Tags” table with:

Its UID

A per-tag Doorbell URL (initialized to whatever the current default URL is)

5. Editing Tag Actions (Per-Tag URL Table)

At the bottom of the page you have a table:

UID	Doorbell URL	Actions
04A1B2C3D4	https://.../frontdoor	Save / [delete]
DEADBEEF01	https://.../garage	Save / [delete]
...	...	...

For each tag:

Change the URL in the input box.

Click “Save” to store the new URL for that tag.

Click “[delete]” to completely remove that tag and its URL from memory.

URLs and tags are all stored persistently in the ESP32’s flash (Preferences), so they survive reboots.

🔔 How Scanning Works

When a tag is tapped on the PN532:

The PN532 raises an IRQ.

The ESP32 reads the card UID.

The code:

Looks through stored tags in Preferences.

If the UID is found:

Loads the tag’s custom URL.

Calls triggerDoorbell(urlForTag) → HTTP GET to that URL.

Logs a message like:

KNOWN TAG → Triggering doorbell with custom URL!
Doorbell triggered, HTTP 200


If the UID is not found:

Logs:

Unknown tag (not stored).


Debounce logic ensures each tag tap only triggers once every DELAY_BETWEEN_CARDS milliseconds.

🧠 Storage Layout (Preferences)

The project uses three Preferences namespaces:

wifi namespace

ssid → stored Wi-Fi network name

password → stored Wi-Fi password

config namespace

doorbellURL → default doorbell URL used:

For the Doorbell Test button

For new tags (initial value)

rfid namespace

count → number of stored tags

waiting → temporary flag:

true means user clicked “Add Next RFID Scan” and next tag UID should be saved

For each tag index i (0..count-1):

tag{i} → UID string (e.g. "04A1B2C3D4")

url{i} → per-tag URL string

🧰 Troubleshooting
❌ “Didn’t find PN53x board”

Check PN532 is in I²C mode (solder pads / jumpers).

Verify wiring:

SDA → D6

SCL → D5

IRQ → D3

RST → D6 (or as your code/hardware uses)

GND shared

Correct VCC (3.3V or 5V depending on board)

Confirm the simple PN532 example from Adafruit or your earlier known-working sketch runs correctly with exactly the same wiring.

❌ Web page doesn’t load

If connected via AP:

Use http://192.168.4.1/.

If connected via your router:

Check Serial Monitor for WiFi connected, IP: x.x.x.x.

Use that IP in your browser.

Make sure your phone/PC is on the same network as the ESP32.

❌ Doorbell not triggering

Check in Serial Monitor that:

The card is recognized as KNOWN TAG.

The HTTP status code is 200 or similar.

Verify the URL:

Paste it into a normal browser to see if it returns a valid page / triggers your automation.

If using Virtual Smart Home:

Make sure the routine URL is correctly copied and the token is valid.

💡 Ideas & Extensions

Things you might add next:

Buzzer / LED feedback

Short beep or green LED for authorized tag

Long beep or red LED for unknown tag

Relay / Door Strike

Turn the ESP32 into an NFC door unlock system as well as a doorbell

Last activity log

Show “Last scanned UID” and timestamp in the web UI

Add a “Recent activity” list

Simple HTTP API

Expose routes like /api/tags and /api/tags/add to manage tags via scripts or a mobile app

Basic auth on the portal

Protect the config page with a username/password
