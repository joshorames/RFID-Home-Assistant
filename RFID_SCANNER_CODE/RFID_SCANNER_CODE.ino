/*******************************************************
   WIFI PORTAL + RFID STORAGE + ALEXA DOORBELL + PN532
   PN532 CODE LEFT CORE-LOGIC IDENTICAL TO YOUR WORKING VERSION
********************************************************/

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// ---------------- PN532 ----------------
#define PN532_IRQ   (D3)
#define PN532_RESET (D6)

const int DELAY_BETWEEN_CARDS = 500;
long timeLastCardRead = 0;
boolean readerDisabled = false;
int irqCurr;
int irqPrev;

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// ---------------- WIFI / PREFS / SERVER ----------------
Preferences prefs;
WebServer server(80);

String ssid, password;

// Default URL will be loaded from prefs ("config" namespace)
String doorbellURL;

// Forward declarations
void startListeningToNFC();
void handleCardDetected();
bool isStoredTag(String uid);   // still available if you want it
void saveNewTag(String uid);
void triggerDoorbell();
void triggerDoorbell(String urlOverride);

// ---------------- WIFI AP MODE ----------------
void startAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Setup", "12345678");
  Serial.print("AP Started: ");
  Serial.println(WiFi.softAPIP());
}

// ---------------- HTML UI ----------------
void handleRoot() {
  bool connected = WiFi.status() == WL_CONNECTED;

  String html = "<html><body style='font-family:sans-serif;'>";
  html += "<h2>ESP32 NFC Doorbell</h2>";

  html += "<p><b>WiFi:</b> ";
  html += connected ? "<span style='color:green;'>Connected</span>"
                    : "<span style='color:red;'>Not Connected</span>";
  html += "</p>";

  // --- Default Doorbell URL section ---
  html += "<h3>Default Doorbell URL</h3>";
  html += "<form action='/save_url' method='POST'>";
  html += "URL:<br><input name='doorbell' size='80' value='" + doorbellURL + "'><br><br>";
  html += "<input type='submit' value='Save Default URL'>";
  html += "</form><hr>";

  // --- WiFi setup ---
  html += "<h3>WiFi Configuration</h3>";
  html += "<form action='/save' method='POST'>";
  html += "SSID:<br><input name='ssid' value='" + ssid + "'><br>";
  html += "Password:<br><input type='password' name='password' value='" + password + "'><br><br>";
  html += "<input type='submit' value='Save WiFi'>";
  html += "</form><hr>";

  // --- Doorbell test ---
  html += "<h3>Doorbell Test</h3>";
  html += "<form action='/doorbell' method='POST'>";
  html += "<button style='padding:15px;font-size:18px;'>Test Doorbell</button>";
  html += "</form><hr>";

  // --- Add RFID ---
  html += "<h3>Add RFID Tag</h3>";
  html += "<form action='/add_rfid' method='POST'>";
  html += "<button style='padding:15px;font-size:18px;'>Add Next RFID Scan</button>";
  html += "</form><hr>";

  // --- RFID table with URLs ---
  html += "<h3>Stored RFID Tags</h3>";
  html += "<table border='1' cellpadding='5' cellspacing='0'>";
  html += "<tr><th>UID</th><th>Doorbell URL</th><th>Actions</th></tr>";

  prefs.begin("rfid", true);
  int count = prefs.getInt("count", 0);
  for (int i = 0; i < count; i++) {
    String tagKey = "tag" + String(i);
    String urlKey = "url" + String(i);
    String tag = prefs.getString(tagKey.c_str(), "");
    if (tag == "") continue;
    String url = prefs.getString(urlKey.c_str(), doorbellURL);

    html += "<tr>";
    html += "<td>" + tag + "</td>";
    html += "<td>";
    html += "<form style='margin:0;' action='/update_rfid' method='POST'>";
    html += "<input type='hidden' name='i' value='" + String(i) + "'>";
    html += "<input name='url' size='60' value='" + url + "'>";
    html += "</td><td>";
    html += "<input type='submit' value='Save'>";
    html += " <a style='color:red' href='/delete_rfid?i=" + String(i) + "'>[delete]</a>";
    html += "</form>";
    html += "</td></tr>";
  }
  prefs.end();

  html += "</table>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

// Save WiFi
void handleSave() {
  ssid = server.arg("ssid");
  password = server.arg("password");

  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.end();

  server.send(200, "text/html",
              "<h3>Saved. Reconnecting...</h3><a href='/'>Back</a>");

  WiFi.disconnect(true);
  delay(300);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());
}

// Save default doorbell URL
void handleSaveURL() {
  doorbellURL = server.arg("doorbell");

  prefs.begin("config", false);
  prefs.putString("doorbellURL", doorbellURL);
  prefs.end();

  server.send(200, "text/html",
              "<h3>Doorbell URL saved.</h3><a href='/'>Back</a>");
}

void handleDoorbell() {
  triggerDoorbell();
  server.send(200, "text/html", "<h3>Doorbell Triggered!</h3><a href='/'>Back</a>");
}

// Click "Add Next RFID Scan"
void handleAddRFID() {
  prefs.begin("rfid", false);
  prefs.putBool("waiting", true);
  prefs.end();

  server.send(200, "text/html",
              "<h3>Scan an RFID tag now...</h3><a href='/'>Back</a>");
}

// Update RFID URL row
void handleUpdateRFID() {
  if (!server.hasArg("i") || !server.hasArg("url")) {
    server.send(400, "text/html", "<h3>Missing data.</h3><a href='/'>Back</a>");
    return;
  }

  int index = server.arg("i").toInt();
  String url = server.arg("url");

  prefs.begin("rfid", false);
  int count = prefs.getInt("count", 0);
  if (index < 0 || index >= count) {
    prefs.end();
    server.send(400, "text/html", "<h3>Invalid index.</h3><a href='/'>Back</a>");
    return;
  }

  prefs.putString(("url" + String(index)).c_str(), url);
  prefs.end();

  server.send(200, "text/html", "<h3>RFID updated.</h3><a href='/'>Back</a>");
}

// Delete RFID tag row
void handleDeleteRFID() {
  int index = server.arg("i").toInt();

  prefs.begin("rfid", false);
  int count = prefs.getInt("count", 0);

  if (index < 0 || index >= count) {
    prefs.end();
    server.send(400, "text/html", "<h3>Invalid index.</h3><a href='/'>Back</a>");
    return;
  }

  for (int j = index; j < count - 1; j++) {
    String nextTag = prefs.getString(("tag" + String(j + 1)).c_str(), "");
    String nextUrl = prefs.getString(("url" + String(j + 1)).c_str(), "");
    prefs.putString(("tag" + String(j)).c_str(), nextTag);
    prefs.putString(("url" + String(j)).c_str(), nextUrl);
  }

  prefs.putInt("count", count - 1);
  prefs.end();

  server.send(200, "text/html", "<h3>Tag deleted.</h3><a href='/'>Back</a>");
}

// ---------------- RFID STORAGE (simple UID lookup still available) ----------------
bool isStoredTag(String uid) {
  prefs.begin("rfid", true);
  int count = prefs.getInt("count", 0);

  for (int i = 0; i < count; i++) {
    if (prefs.getString(("tag" + String(i)).c_str(), "") == uid) {
      prefs.end();
      return true;
    }
  }
  prefs.end();
  return false;
}

void saveNewTag(String uid) {
  prefs.begin("rfid", false);
  int count = prefs.getInt("count", 0);
  prefs.putString(("tag" + String(count)).c_str(), uid);
  // Give new tags the current default doorbell URL
  prefs.putString(("url" + String(count)).c_str(), doorbellURL);
  prefs.putInt("count", count + 1);
  prefs.putBool("waiting", false);
  prefs.end();

  Serial.print("Saved new tag: ");
  Serial.println(uid);
}

// ---------------- DOORBELL ----------------
void triggerDoorbell() {
  triggerDoorbell(doorbellURL);
}

void triggerDoorbell(String urlOverride) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected.");
    return;
  }

  HTTPClient http;
  http.begin(urlOverride);
  int code = http.GET();
  http.end();

  if (code > 0) {
    Serial.print("Doorbell triggered, HTTP ");
    Serial.println(code);
  } else {
    Serial.print("Doorbell failed, HTTP ");
    Serial.println(code);
  }
}

// ---------------- PN532 (BASED ON YOUR ORIGINAL WORKING CODE) ----------------
void startListeningToNFC() {
  irqPrev = irqCurr = HIGH;

  Serial.println("Starting passive read for ISO14443A card...");
  if (!nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
    Serial.println("No card found. Waiting...");
  } else {
    Serial.println("Card already present.");
    handleCardDetected();
  }
}

void handleCardDetected() {
  uint8_t uid[7];
  uint8_t uidLength;
  uint8_t success = nfc.readDetectedPassiveTargetID(uid, &uidLength);

  Serial.println(success ? "Read successful" : "Read failed");

  if (success) {
    String uidStr = "";

    for (int i = 0; i < uidLength; i++) {
      char buffer[3];
      sprintf(buffer, "%02X", uid[i]);
      uidStr += buffer;
    }

    Serial.print("Card UID: ");
    Serial.println(uidStr);

    // ADD MODE?
    prefs.begin("rfid", false);
    bool waiting = prefs.getBool("waiting", false);
    prefs.end();

    if (waiting) {
      saveNewTag(uidStr);
    } else {
      // Look up custom URL for this UID
      prefs.begin("rfid", true);
      int count = prefs.getInt("count", 0);
      String urlForTag = "";

      for (int i = 0; i < count; i++) {
        String tagKey = "tag" + String(i);
        String tag = prefs.getString(tagKey.c_str(), "");
        if (tag == uidStr) {
          String urlKey = "url" + String(i);
          urlForTag = prefs.getString(urlKey.c_str(), doorbellURL);
          break;
        }
      }
      prefs.end();

      if (urlForTag.length() > 0) {
        Serial.println("KNOWN TAG → Triggering doorbell with custom URL!");
        triggerDoorbell(urlForTag);
      } else {
        Serial.println("Unknown tag (not stored).");
      }
    }

    timeLastCardRead = millis();
  }

  readerDisabled = true;
}

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(1);

  // --- Load WiFi creds ---
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  password = prefs.getString("password", "");
  prefs.end();

  // --- Load default doorbell URL ---
  prefs.begin("config", true);
  doorbellURL = prefs.getString(
    "doorbellURL",
    "https://www.virtualsmarthome.xyz/url_routine_trigger/activate.php?trigger=c982e4bd-a972-447e-a583-a93217ea0484&token=a48016af-88a3-4773-ba7f-1d780bfc40b9&response=html"
  );
  prefs.end();

  // --- Init RFID prefs defaults ---
  prefs.begin("rfid", false);
  if (!prefs.isKey("count")) {
    prefs.putInt("count", 0);
  }
  prefs.putBool("waiting", false);
  prefs.end();

  // --- WiFi startup behavior ---
  if (ssid == "") {
    // First-time setup → AP only
    Serial.println("No saved WiFi. Starting AP setup.");
    startAP();
  } else {
    Serial.println("Connecting to saved WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 10000) {
      delay(250);
      Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("WiFi connected, IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi connect failed. Enabling AP+STA for config.");
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP("ESP32-Setup", "12345678");
    }
  }

  // --- PN532 init (same core logic as your working code) ---
  Serial.println("Hello!");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }
  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);

  startListeningToNFC();

  // --- Web routes ---
  server.on("/", handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/save_url", HTTP_POST, handleSaveURL);
  server.on("/doorbell", HTTP_POST, handleDoorbell);
  server.on("/add_rfid", HTTP_POST, handleAddRFID);
  server.on("/update_rfid", HTTP_POST, handleUpdateRFID);
  server.on("/delete_rfid", handleDeleteRFID);

  server.begin();
}

// ---------------- LOOP ----------------
void loop() {
  server.handleClient();

  if (readerDisabled) {
    if (millis() - timeLastCardRead > DELAY_BETWEEN_CARDS) {
      readerDisabled = false;
      startListeningToNFC();
    }
  } else {
    irqCurr = digitalRead(PN532_IRQ);
    if (irqCurr == LOW && irqPrev == HIGH) {
      Serial.println("Got NFC IRQ");
      handleCardDetected();
    }
    irqPrev = irqCurr;
  }
}
