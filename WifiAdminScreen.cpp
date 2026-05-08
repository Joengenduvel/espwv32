#include "GenericScreen.h"
#include "Storage.h"
#include "ble.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_wifi.h"
#include "esp_coexist.h"
#include "esp_random.h"

#define WIFI_ADMIN_SSID "ESPWV32-Admin"
#define DNS_PORT 53

namespace espwv32 {

class WifiAdminScreen : public GenericScreen {
  public:
    WifiAdminScreen(ble::BLEKeyboard* keyboard) {
      _keyboard = keyboard;
      _storage = new Storage();
      memset(_userPin, 0, 4);
    }
    ~WifiAdminScreen() {
      stopAP();
      delete _storage;
    }

    void updatePin(uint8_t* pin) {
      memcpy(_userPin, pin, 4);
      reset();
    }

    ScreenType getType() {
      return WIFI_ADMIN;
    }

    // Called every loop() iteration to process web + DNS requests
    void handle() override {
      if (_server != nullptr) _server->handleClient();
      if (_dns   != nullptr) _dns->processNextRequest();
      // If _toNextScreen was set by an HTTP handler, the server is still
      // running (we can't call stopAP() inside a server callback).
      // Clean up here, one loop iteration after the response was delivered.
      if (_toNextScreen && _server != nullptr) stopAP();
    }

    // Button A (any duration) → stop AP and return to account list
    void buttonPressedA()      override { _exitToAccounts(); }
    void buttonMediumPressedA() override { _exitToAccounts(); }
    void buttonLongPressedA()  override { _exitToAccounts(); }

  private:
    Storage*          _storage  = nullptr;
    ble::BLEKeyboard* _keyboard = nullptr;
    uint8_t           _userPin[4];
    WebServer* _server      = nullptr;
    DNSServer* _dns         = nullptr;
    String     _wifiPass;

    // Generates a 12-character random password using unambiguous characters
    static String generatePass() {
      const char chars[] = "abcdefghjkmnpqrstuvwxyz23456789"; // 31 chars (no 0/O/l/1/I)
      uint8_t rnd[12];
      esp_fill_random(rnd, sizeof(rnd));
      String pass;
      for (int i = 0; i < 12; i++) pass += chars[rnd[i] % 31];
      return pass;
    }

    void _exitToAccounts() {
      stopAP();
      _toNextScreen = true;
    }

    void show() override {
      // Draw "starting..." placeholder first so the screen isn't blank during AP init
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("WiFi Admin");
      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.setCursor(0, 20);
      M5.Lcd.print("Starting AP...");

      startAP();   // blocks until AP is ready (max 3 s)
      drawScreen(); // redraw with actual IP
    }

    void drawScreen() {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);

      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("WiFi Admin");

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(0, 20);
      M5.Lcd.print("SSID: " WIFI_ADMIN_SSID);
      M5.Lcd.setCursor(0, 30);
      M5.Lcd.print("Pass: ");
      M5.Lcd.print(_wifiPass);
      M5.Lcd.setCursor(0, 42);
      M5.Lcd.print("IP:   ");
      M5.Lcd.print(WiFi.softAPIP().toString());

      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.setCursor(0, 58);
      M5.Lcd.print("[A] Back");
    }

    void startAP() {
      if (_server != nullptr) return; // already running

      setCpuFrequencyMhz(240);

      // Disconnect any active BLE connection and stop advertising.
      // An active BLE connection does continuous 37-channel frequency hopping
      // which overwhelms the coexistence module and prevents DHCP packets
      // from being delivered. Advertising-only is not enough — the connection
      // itself must be dropped.
      if (_keyboard != nullptr) {
        if (_keyboard->isConnected()) _keyboard->disconnect();
        _keyboard->stopAdvertising();
      }
      // Give WiFi priority in the BLE/WiFi coexistence scheduler
      esp_coex_preference_set(ESP_COEX_PREFER_WIFI);
      delay(200);

      WiFi.persistent(false);
      WiFi.mode(WIFI_AP);
      delay(200);

      // Load stored WiFi password; generate and persist one if not yet set
      _wifiPass = _storage->getWifiPass();
      if (_wifiPass.length() == 0) {
        _wifiPass = generatePass();
        _storage->setWifiPass(_wifiPass);
      }

      WiFi.softAP(WIFI_ADMIN_SSID, _wifiPass.c_str(), 6);
      // Disable WiFi modem sleep to prevent dropped data frames
      esp_wifi_set_ps(WIFI_PS_NONE);
      delay(500);

      // Wait until the AP IP is ready, max 3 s
      unsigned long t = millis();
      while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && millis() - t < 3000) {
        delay(50);
      }

      _dns = new DNSServer();
      _dns->start(DNS_PORT, "*", WiFi.softAPIP());

      _server = new WebServer(80);
      _server->onNotFound([this]() {
        _server->sendHeader("Location", "http://192.168.4.1/", true);
        _server->send(302, "text/plain", "");
      });
      _server->begin();

      // When all clients disconnect, return to the account screen automatically
      WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        if (WiFi.softAPgetStationNum() == 0) {
          _exitToAccounts();
        }
      }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
    }

    void stopAP() {
      if (_server != nullptr) { _server->stop(); delete _server; _server = nullptr; }
      if (_dns    != nullptr) { _dns->stop();    delete _dns;    _dns    = nullptr; }
      WiFi.softAPdisconnect(true);
      WiFi.mode(WIFI_OFF);
      setCpuFrequencyMhz(80);
      esp_coex_preference_set(ESP_COEX_PREFER_BALANCE);
      if (_keyboard != nullptr) _keyboard->startAdvertising();
    }

    // ---------- web handlers ----------

    void handleRoot() {
      String status = _server->arg("status");
      _server->send(200, "text/html", buildPage(status));
    }

    void handleChangePin() {
      // Parse each 4-digit PIN from individual digit fields
      auto parsePin = [this](const char* prefix, uint8_t out[4]) {
        for (int i = 0; i < 4; i++) {
          String key = String(prefix) + String(i);
          out[i] = (uint8_t)constrain(_server->arg(key).toInt(), 0, 9);
        }
      };

      uint8_t current[4], next[4], confirm[4];
      parsePin("cur", current);
      parsePin("new", next);
      parsePin("cfm", confirm);

      if (memcmp(next, confirm, 4) != 0) {
        _server->sendHeader("Location", "/?status=pin_mismatch", true);
        _server->send(302, "text/plain", "");
        return;
      }
      if (!_storage->resetPin(current, next)) {
        _server->sendHeader("Location", "/?status=pin_wrong", true);
        _server->send(302, "text/plain", "");
        return;
      }
      memcpy(_userPin, next, 4); // update session PIN so saved accounts still decrypt
      _server->sendHeader("Location", "/?status=pin_ok", true);
      _server->send(302, "text/plain", "");
    }

    void handleResetWifiPass() {
      _wifiPass = generatePass();
      _storage->setWifiPass(_wifiPass);
      drawScreen(); // show the new password on the device screen before AP closes
      // Send a final page to the browser — AP stops on the next handle() iteration
      _server->send(200, "text/html",
        "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body>"
        "<h2>&#x1F4F6; New WiFi password generated</h2>"
        "<p>The new password is shown on the device screen.</p>"
        "<p>The access point is now closing.</p>"
        "</body></html>");
      _toNextScreen = true; // handle() will call stopAP() on the next iteration
    }

    void handleDelete() {      int idx = _server->arg("index").toInt();
      if (idx < 0 || idx >= Storage::maxSlots()) {
        _server->send(400, "text/plain", "Invalid index");
        return;
      }
      _storage->deleteSlot(idx);
      _server->sendHeader("Location", "/", true);
      _server->send(302, "text/plain", "");
    }

    void handleSave() {
      int idx = _server->arg("index").toInt();
      if (idx < 0 || idx >= Storage::maxSlots()) {
        _server->send(400, "text/plain", "Invalid index");
        return;
      }
      Credentials creds;
      memset(&creds, 0, sizeof(creds));
      _server->arg("name")    .toCharArray(creds.name,     sizeof(creds.name));
      _server->arg("username").toCharArray(creds.username, sizeof(creds.username));
      _server->arg("password").toCharArray(creds.password, sizeof(creds.password));
      _storage->store(idx, creds, _userPin);
      EEPROM.commit();
      _server->sendHeader("Location", "/", true);
      _server->send(302, "text/plain", "");
    }

    // ---------- page builder ----------

    String buildPage(String status = "") {
      String html =
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>ESPWV32 Admin</title>"
        "<style>"
        "body{font-family:sans-serif;max-width:560px;margin:24px auto;padding:0 12px;background:#f4f4f4}"
        "h1{font-size:1.4em;color:#333}"
        "h2{font-size:1em;color:#555;margin:0 0 8px}"
        ".card{background:#fff;border-radius:6px;padding:12px;margin:10px 0;box-shadow:0 1px 3px rgba(0,0,0,.15)}"
        "label{display:block;font-size:.8em;color:#666;margin-top:6px}"
        "input[type=text],input[type=password]{width:100%;padding:6px;box-sizing:border-box;border:1px solid #ccc;border-radius:3px}"
        ".pin-row{display:flex;gap:6px}"
        ".pin-row input{width:48px;text-align:center;padding:6px;border:1px solid #ccc;border-radius:3px;font-size:1.2em}"
        "button{margin-top:8px;background:#0069d9;color:#fff;border:none;padding:7px 18px;border-radius:3px;cursor:pointer}"
        "button:hover{background:#0053aa}"
        "button.del{background:#c00;margin-left:6px}"
        "button.del:hover{background:#900}"
        ".btn-row{display:flex;align-items:center}"
        ".ok{color:#1a7f1a;font-weight:bold;margin-bottom:8px}"
        ".err{color:#c00;font-weight:bold;margin-bottom:8px}"
        "</style></head><body>"
        "<h1>&#x1F512; ESPWV32 Account Manager</h1>";

      // ── Status banner ──────────────────────────────────────────────────────
      if (status == "pin_ok")
        html += "<p class='ok'>&#10003; PIN changed successfully.</p>";
      else if (status == "pin_wrong")
        html += "<p class='err'>&#10007; Current PIN incorrect.</p>";
      else if (status == "pin_mismatch")
        html += "<p class='err'>&#10007; New PINs do not match.</p>";

      // ── Change PIN card ────────────────────────────────────────────────────
      auto pinInputs = [](const char* prefix, const char* legend) -> String {
        String s = String("<label>") + legend + "</label><div class='pin-row'>";
        for (int i = 0; i < 4; i++)
          s += "<input type='number' name='" + String(prefix) + String(i) +
               "' min='0' max='9' value='0' required>";
        return s + "</div>";
      };

      html += "<div class='card'><h2>&#x1F511; Change PIN</h2>"
              "<form method='POST' action='/change-pin'>" +
              pinInputs("cur", "Current PIN") +
              pinInputs("new", "New PIN") +
              pinInputs("cfm", "Confirm New PIN") +
              "<button type='submit'>Change PIN</button>"
              "</form></div>";

      // ── Account slots ──────────────────────────────────────────────────────
      uint8_t slotCount   = _storage->getSlotCount();
      uint8_t slotsToShow = (uint8_t)min((int)slotCount + 1, Storage::maxSlots());

      for (int i = 0; i < slotsToShow; i++) {
        // Slots 0 .. slotCount-1 are saved — read and decrypt from EEPROM.
        // Slot slotCount is a blank entry for adding a new account — show empty fields.
        Credentials c;
        if (i < slotCount) {
          c = _storage->read(i, _userPin);
        } else {
          memset(&c, 0, sizeof(c));
        }
        html += "<div class='card'>"
                "<form method='POST' action='/save'>"
                "<input type='hidden' name='index' value='" + String(i) + "'>"
                "<b>Slot " + String(i) + "</b>"
                "<label>Name</label>"
                "<input type='text' name='name' value='" + esc(c.name) + "'>"
                "<label>Username</label>"
                "<input type='text' name='username' value='" + esc(c.username) + "'>"
                "<label>Password</label>"
                "<input type='password' name='password' value='" + esc(c.password) + "'>"
                "<div class='btn-row'>"
                "<button type='submit'>Save</button>";
        // Only saved slots can be deleted (not the blank new-entry slot)
        if (i < slotCount) {
          html += "<form method='POST' action='/delete' style='margin-top:8px'>"
                  "<input type='hidden' name='index' value='" + String(i) + "'>"
                  "<button class='del' type='submit' "
                  "onclick=\"return confirm('Delete slot " + String(i) + "?')\""
                  ">Delete</button>"
                  "</form>";
        }
        html += "</div></form></div>";
      }

      html += "</body></html>";
      return html;
    }

    // Escape HTML special characters to prevent XSS in attribute values
    String esc(const char* s) {
      String out(s);
      out.replace("&",  "&amp;");
      out.replace("<",  "&lt;");
      out.replace(">",  "&gt;");
      out.replace("\"", "&quot;");
      out.replace("'",  "&#39;");
      return out;
    }
};
}

