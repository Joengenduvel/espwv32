#include "GenericScreen.h"
#include "Storage.h"
#include "ble.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_wifi.h"
#include "esp_coexist.h"

#define WIFI_ADMIN_SSID "ESPWV32-Admin"
#define WIFI_ADMIN_PASS "vault1234"
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
    }

    // Button A (any duration) → stop AP and return to account list
    void buttonPressedA()      override { _exitToAccounts(); }
    void buttonMediumPressedA() override { _exitToAccounts(); }
    void buttonLongPressedA()  override { _exitToAccounts(); }

  private:
    Storage*          _storage  = nullptr;
    ble::BLEKeyboard* _keyboard = nullptr;
    uint8_t           _userPin[4];
    WebServer* _server  = nullptr;
    DNSServer* _dns     = nullptr;

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
      M5.Lcd.print("Pass: " WIFI_ADMIN_PASS);
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

      WiFi.softAP(WIFI_ADMIN_SSID, WIFI_ADMIN_PASS, 6);
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
      _server->on("/",      HTTP_GET,  [this]() { handleRoot(); });
      _server->on("/save",  HTTP_POST, [this]() { handleSave(); });
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
      _server->send(200, "text/html", buildPage());
    }

    void handleSave() {
      int idx = _server->arg("index").toInt();
      if (idx < 0 || idx >= 10) {
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

    String buildPage() {
      String html =
        "<!DOCTYPE html><html><head>"
        "<meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>ESPWV32 Admin</title>"
        "<style>"
        "body{font-family:sans-serif;max-width:560px;margin:24px auto;padding:0 12px;background:#f4f4f4}"
        "h1{font-size:1.4em;color:#333}"
        ".card{background:#fff;border-radius:6px;padding:12px;margin:10px 0;box-shadow:0 1px 3px rgba(0,0,0,.15)}"
        "label{display:block;font-size:.8em;color:#666;margin-top:6px}"
        "input[type=text],input[type=password]{width:100%;padding:6px;box-sizing:border-box;border:1px solid #ccc;border-radius:3px}"
        "button{margin-top:8px;background:#0069d9;color:#fff;border:none;padding:7px 18px;border-radius:3px;cursor:pointer}"
        "button:hover{background:#0053aa}"
        "</style></head><body>"
        "<h1>&#x1F512; ESPWV32 Account Manager</h1>";

      for (int i = 0; i < 10; i++) {
        Credentials c = _storage->read(i, _userPin);
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
                "<button type='submit'>Save</button>"
                "</form></div>";
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

