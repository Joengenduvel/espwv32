#include "WifiAdmin.h"

using namespace espwv32;

WifiAdmin::WifiAdmin() {
  _storage = new Storage();
  memset(_userPin, 0, 4);
}

WifiAdmin::~WifiAdmin() {
  stop();
  delete _storage;
}

void WifiAdmin::updatePin(uint8_t pin[]) {
  memcpy(_userPin, pin, 4);
}

// ── AP lifecycle ──────────────────────────────────────────────────────────────

void WifiAdmin::start() {
  if (_server != nullptr) return; // already running

  setCpuFrequencyMhz(240);

  esp_coex_preference_set(ESP_COEX_PREFER_WIFI);
  delay(200);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  delay(200);

  // Load stored WiFi password; generate and persist one on first boot
  _wifiPass = _storage->getWifiPass();
  if (_wifiPass.length() == 0) {
    _wifiPass = generatePass();
    _storage->setWifiPass(_wifiPass);
  }

  WiFi.softAP(WIFI_ADMIN_SSID, _wifiPass.c_str(), 6);
  esp_wifi_set_ps(WIFI_PS_NONE); // prevent modem sleep dropping data frames
  delay(500);

  unsigned long t = millis();
  while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && millis() - t < 3000) {
    delay(50);
  }

  _dns = new DNSServer();
  _dns->start(DNS_PORT, "*", WiFi.softAPIP());

  _server = new WebServer(80);
  _server->on("/",                HTTP_GET,  [this]() { handleRoot(); });
  _server->on("/save",            HTTP_POST, [this]() { handleSave(); });
  _server->on("/delete",          HTTP_POST, [this]() { handleDelete(); });
  _server->on("/change-pin",      HTTP_POST, [this]() { handleChangePin(); });
  _server->on("/reset-wifi-pass", HTTP_POST, [this]() { handleResetWifiPass(); });
  _server->onNotFound([this]() {
    _server->sendHeader("Location", "http://192.168.4.1/", true);
    _server->send(302, "text/plain", "");
  });
  _server->begin();

  // When all clients disconnect, signal the screen to exit
  WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
    if (WiFi.softAPgetStationNum() == 0) _done = true;
  }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void WifiAdmin::stop() {
  if (_server != nullptr) { _server->stop(); delete _server; _server = nullptr; }
  if (_dns    != nullptr) { _dns->stop();    delete _dns;    _dns    = nullptr; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(80);
  esp_coex_preference_set(ESP_COEX_PREFER_BT);
  // BLE advertising is restarted by WifiAdminScreen after this call
}

void WifiAdmin::handle() {
  if (_server != nullptr) _server->handleClient();
  if (_dns    != nullptr) _dns->processNextRequest();
  // If _done was set by an HTTP handler, the server is still running — stop
  // it now, one iteration after the response was delivered (not inside callback)
  if (_done && _server != nullptr) stop();
}

// ── Web handlers ──────────────────────────────────────────────────────────────

void WifiAdmin::handleRoot() {
  _server->send(200, "text/html", buildPage(_server->arg("status")));
}

void WifiAdmin::handleSave() {
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

void WifiAdmin::handleDelete() {
  int idx = _server->arg("index").toInt();
  if (idx < 0 || idx >= Storage::maxSlots()) {
    _server->send(400, "text/plain", "Invalid index");
    return;
  }
  _storage->deleteSlot(idx);
  _server->sendHeader("Location", "/", true);
  _server->send(302, "text/plain", "");
}

void WifiAdmin::handleChangePin() {
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
  memcpy(_userPin, next, 4);
  _server->sendHeader("Location", "/?status=pin_ok", true);
  _server->send(302, "text/plain", "");
}

void WifiAdmin::handleResetWifiPass() {
  _wifiPass = generatePass();
  _storage->setWifiPass(_wifiPass);
  _passChanged = true; // screen will redraw on next handle() check
  _server->send(200, "text/html",
    "<!DOCTYPE html><html><head><meta charset='UTF-8'></head><body>"
    "<h2>&#x1F4F6; New WiFi password generated</h2>"
    "<p>The new password is shown on the device screen.</p>"
    "<p>The access point is now closing.</p>"
    "</body></html>");
  _done = true; // handle() will call stop() on the next iteration
}

// ── Page builder ──────────────────────────────────────────────────────────────

String WifiAdmin::generatePass() {
  const char chars[] = "abcdefghjkmnpqrstuvwxyz23456789"; // 31 unambiguous chars
  uint8_t rnd[12];
  esp_fill_random(rnd, sizeof(rnd));
  String pass;
  for (int i = 0; i < 12; i++) pass += chars[rnd[i] % 31];
  return pass;
}

String WifiAdmin::buildPage(String status) {
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

  // Status banner
  if      (status == "pin_ok")       html += "<p class='ok'>&#10003; PIN changed.</p>";
  else if (status == "pin_wrong")    html += "<p class='err'>&#10007; Current PIN incorrect.</p>";
  else if (status == "pin_mismatch") html += "<p class='err'>&#10007; New PINs do not match.</p>";

  // WiFi password card
  html += "<div class='card'><h2>&#x1F4F6; WiFi Admin Password</h2>"
          "<p style='font-size:.9em;color:#555;margin:4px 0'>SSID: <b>" WIFI_ADMIN_SSID "</b></p>"
          "<form method='POST' action='/reset-wifi-pass'>"
          "<button type='submit' "
          "onclick=\"return confirm('Generate a new WiFi password? You will need to reconnect.')\""
          ">Generate new password</button>"
          "</form></div>";

  // Change PIN card
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

  // Account slots
  uint8_t slotCount   = _storage->getSlotCount();
  uint8_t slotsToShow = (uint8_t)min((int)slotCount + 1, Storage::maxSlots());

  for (int i = 0; i < slotsToShow; i++) {
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

String WifiAdmin::esc(const char* s) {
  String out(s);
  out.replace("&",  "&amp;");
  out.replace("<",  "&lt;");
  out.replace(">",  "&gt;");
  out.replace("\"", "&quot;");
  out.replace("'",  "&#39;");
  return out;
}

