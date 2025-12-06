ESP32 FRID Home Assistant
Web Portal • NFC Tag Storage • Per-Tag Actions • PN532 Reader • Alexa-Compatible

This project turns an ESP32 board and a PN532 NFC module into a configurable smart doorbell or access automation system. Each NFC/RFID tag can trigger its own custom URL (such as an Alexa Virtual Smart Home routine, Home Assistant webhook, or any HTTP GET endpoint).

Everything is controlled from a built-in web interface: Wi-Fi setup, NFC tag management, and URL assignment.

Features
Web-Based Control Panel

Configure Wi-Fi (SSID and password)

Edit the default doorbell URL

Add new NFC/RFID tags

Edit per-tag URLs

Delete tags

Trigger a test doorbell action

NFC / RFID Tag Reader

Uses PN532 in I2C + IRQ mode

Reads card UID reliably

Debounce system prevents repeated triggers

Unknown tags are ignored

Known tags trigger their assigned URL via HTTP GET

Persistent Storage

Stored using ESP32 Preferences (flash memory):

Wi-Fi credentials

Default doorbell URL

List of NFC tags

Per-tag URLs

Add-next-tag mode flag

Smart Wi-Fi Boot Logic

First boot: Access Point mode (SSID: ESP32-Setup, password: 12345678)

Normal boot: Connects with saved Wi-Fi credentials

If unable to connect: AP + STA mode (still configurable while attempting to reconnect)

Compatible With:

Alexa Virtual Smart Home (URL routines)

Home Assistant

IFTTT

Node-RED

Any system that accepts HTTP GET requests

Hardware Required

Seeed Studio XIAO ESP32-S3 (recommended)

PN532 NFC Reader (red generic model, I2C mode)

USB-C cable

RFID/NFC cards or key fobs

Wiring
PN532 Pin	XIAO ESP32-S3 Pin	Description
SDA	D6	I2C Data
SCL	D5	I2C Clock
IRQ	D3	Interrupt line
RST / RSTO	D6	Reset pin (matches code)
VCC	3.3V or 5V	Power (depends on module)
GND	GND	Ground

The code assumes:

#define PN532_IRQ   (D3)
#define PN532_RESET (D6)

Setup Instructions
1. Flash the Firmware

Open the .ino file in Arduino IDE, select the correct board and port, and upload the sketch.

2. First-Time Wi-Fi Setup

If no saved Wi-Fi credentials exist, the ESP32 starts in AP mode:

SSID: ESP32-Setup

Password: 12345678

Connect to it, then open:

http://192.168.4.1/


Use the web page to enter your Wi-Fi SSID/password.

3. Configure Default Doorbell URL

In the web interface, set your preferred HTTP GET endpoint such as an Alexa Virtual Smart Home routine URL.

4. Add NFC Tags

In the web page, click “Add Next RFID Scan”

Tap a tag on the PN532 reader

Tag appears in the table with its own URL field

5. Edit or Delete Tags

Each tag row contains:

Its UID

Editable URL input

Save action

Delete action

New tags inherit the default URL but can be assigned unique URLs.

How It Works
NFC Read Cycle

PN532 detects tag and triggers IRQ

ESP32 reads UID

If “add mode” is active → tag is saved

If tag UID exists → its assigned URL is loaded

URL is called using HTTP GET

Wi-Fi Cycle

ESP32 loads saved SSID/password

Attempts STA connection for ~10 seconds

If connection succeeds → normal mode

If connection fails → AP+STA mode (allows configuration)

Storage Layout (Preferences)
Namespace: wifi

ssid

password

Namespace: config

doorbellURL

Namespace: rfid

count

waiting

tag0, tag1, tag2, …

url0, url1, url2, …

Each tag index matches its URL entry.

Troubleshooting
PN532 Not Detected

Ensure PN532 is in I2C mode (check solder jumpers)

Verify wiring:

SDA → D6

SCL → D5

IRQ → D3

RST → D6

Try the standalone PN532 example sketch to verify hardware

Cannot Access Web Interface

In AP mode → use http://192.168.4.1/

In STA mode → check Serial Monitor for the assigned IP

Doorbell Not Triggering

Check that the tag appears as “KNOWN TAG” in Serial Monitor

Verify the assigned URL manually in a browser

Confirm Internet connectivity

Extensions and Add-Ons

You may extend the project with:

Relay for door strike / lock

LED status indicators (authorized / unauthorized)

Buzzer feedback

Last-N scanned tags log in the interface

MQTT integration

Password-protected web portal

License

This project is released under the MIT License.

