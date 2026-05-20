#include "WifiAdmin.h"
#include "System.cpp"

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
  Serial.printf("[WA] starting AP  free heap: %u\n", esp_get_free_heap_size());
  _done = false;
  _lastHttpActivityMs = millis();

  // Boost CPU while AP + web UI are active; switched back to 80 MHz in stop().
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
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_max_tx_power(44);
  delay(500);

  unsigned long t = millis();
  while (WiFi.softAPIP() == IPAddress(0, 0, 0, 0) && millis() - t < 3000) {
    delay(50);
  }
  if (WiFi.softAPIP() == IPAddress(0, 0, 0, 0)) {
    Serial.println("[WA] ERROR: softAP IP never assigned — captive portal will not work");
  } else {
    Serial.printf("[WA] AP up  IP: %s  free heap: %u\n",
                  WiFi.softAPIP().toString().c_str(), esp_get_free_heap_size());
  }

  _dns = new DNSServer();
  _dns->start(DNS_PORT, "*", WiFi.softAPIP());

  _server = new WebServer(80);
  _server->on("/favicon.ico", HTTP_GET, [this]() {
    const char* svg =
      "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'>"
      "<rect width='64' height='64' rx='12' fill='#0d1117'/>"
      "<rect x='16' y='28' width='32' height='24' rx='4' fill='#1f6feb'/>"
      "<path d='M22 28v-5a10 10 0 0 1 20 0v5' fill='none' stroke='#ffffff' stroke-width='4' stroke-linecap='round'/>"
      "<circle cx='32' cy='40' r='4' fill='#ffffff'/>"
      "</svg>";
    _server->send(200, "image/svg+xml", svg);
  });
  _server->on("/favicon.svg", HTTP_GET, [this]() {
    const char* svg =
      "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 64 64'>"
      "<rect width='64' height='64' rx='12' fill='#0d1117'/>"
      "<rect x='16' y='28' width='32' height='24' rx='4' fill='#1f6feb'/>"
      "<path d='M22 28v-5a10 10 0 0 1 20 0v5' fill='none' stroke='#ffffff' stroke-width='4' stroke-linecap='round'/>"
      "<circle cx='32' cy='40' r='4' fill='#ffffff'/>"
      "</svg>";
    _server->send(200, "image/svg+xml", svg);
  });
  _server->on("/generate_204",      HTTP_GET,  [this]() { _server->send(204, "text/plain", ""); });
  _server->on("/connecttest.txt",   HTTP_GET,  [this]() { _server->sendHeader("Location", "/settings", true); _server->send(302, "text/plain", ""); });
  _server->on("/ncsi.txt",          HTTP_GET,  [this]() { _server->sendHeader("Location", "/settings", true); _server->send(302, "text/plain", ""); });
  // Apple captive portal probes should land in the admin UI, not a static success page.
  _server->on("/hotspot-detect.html", HTTP_GET, [this]() { _server->sendHeader("Location", "/settings", true); _server->send(302, "text/plain", ""); });
  _server->on("/library/test/success.html", HTTP_GET, [this]() { _server->sendHeader("Location", "/settings", true); _server->send(302, "text/plain", ""); });
  _server->on("/success.txt", HTTP_GET, [this]() { _server->sendHeader("Location", "/settings", true); _server->send(302, "text/plain", ""); });
  _server->on("/",                HTTP_GET,  [this]() { handleRoot(); });
  _server->on("/accounts",        HTTP_GET,  [this]() { handleAccountsPage(); });
  _server->on("/accounts",        HTTP_POST, [this]() { handleUpsertAccount(); });
  _server->on("/settings",        HTTP_GET,  [this]() { handleSettingsPage(); });
  _server->on("/battery",         HTTP_GET,  [this]() { handleBattery(); });
  _server->on("/change-pin",      HTTP_POST, [this]() { handleChangePin(); });
  _server->on("/reset-wifi-pass", HTTP_POST, [this]() { handleResetWifiPass(); });
  _server->onNotFound([this]() {
    if (_server->method() == HTTP_DELETE && _server->uri().startsWith("/accounts/")) {
      handleDeleteAccountByPath();
      return;
    }
    _server->sendHeader("Location", "/settings", true);
    _server->send(302, "text/plain", "");
  });
  _server->begin();

  if (!_eventsRegistered) {
    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
      Serial.printf("[WA] STA connected. count=%d\n", WiFi.softAPgetStationNum());
    }, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
      Serial.printf("[WA] STA disconnected. count=%d\n", WiFi.softAPgetStationNum());
      // Do not auto-close admin on disconnect; browsers may reconnect during
      // captive-portal probing or after partial transfers.
    }, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);

    _eventsRegistered = true;
  }
}

void WifiAdmin::stop() {
  if (_server == nullptr && _dns == nullptr) return;
  Serial.println("[WA] stopping AP");
  if (_server != nullptr) { _server->stop(); delete _server; _server = nullptr; }
  if (_dns    != nullptr) { _dns->stop();    delete _dns;    _dns    = nullptr; }
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(80);
  esp_coex_preference_set(ESP_COEX_PREFER_BT);
  Serial.printf("[WA] AP stopped  free heap: %u\n", esp_get_free_heap_size());
  // BLE advertising is restarted by WifiAdminScreen after this call
}

void WifiAdmin::handle() {
  if (_server != nullptr) _server->handleClient();
  if (_dns    != nullptr) _dns->processNextRequest();

  // If _done was set by an HTTP handler, the server is still running — stop
  // it now, one iteration after the response was delivered (not inside callback)
  if (_done && _server != nullptr) stop();
}

void WifiAdmin::markHttpActivity() {
  _lastHttpActivityMs = millis();
}

// ── Web handlers ──────────────────────────────────────────────────────────────

void WifiAdmin::handleRoot() {
  markHttpActivity();
  _server->sendHeader("Location", "/settings", true);
  _server->send(302, "text/plain", "");
}

void WifiAdmin::handleAccountsPage() {
  markHttpActivity();
  Serial.printf("[WA] GET /accounts  free heap: %u\n", esp_get_free_heap_size());
  int selectedSlot = _server->hasArg("slot") ? _server->arg("slot").toInt() : -1;
  String html = buildAccountsPage(_server->arg("status"), selectedSlot);
  Serial.printf("[WA] accounts page size: %u bytes\n", html.length());
  sendHtmlPage(html);
  Serial.println("[WA] accounts page sent");
}

void WifiAdmin::handleSettingsPage() {
  markHttpActivity();
  Serial.printf("[WA] GET /settings  free heap: %u\n", esp_get_free_heap_size());
  String html = buildSettingsPage(_server->arg("status"));
  Serial.printf("[WA] settings page size: %u bytes\n", html.length());
  sendHtmlPage(html);
  Serial.println("[WA] settings page sent");
}

void WifiAdmin::sendHtmlPage(const String& html) {
  WiFiClient client = _server->client();
  if (!client) {
    Serial.println("[WA] ERROR: no active client for HTML response");
    return;
  }

  const size_t len = html.length();
  client.printf(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Content-Length: %u\r\n"
    "Connection: close\r\n\r\n",
    (unsigned)len);

  const uint8_t* data = (const uint8_t*)html.c_str();
  size_t offset = 0;
  while (offset < len) {
    if (!client.connected()) {
      Serial.printf("[WA] ERROR: client disconnected at %u/%u bytes\n",
                    (unsigned)offset, (unsigned)len);
      return;
    }

    size_t chunk = min((size_t)256, len - offset);
    size_t written = client.write(data + offset, chunk);

    if (written == 0) {
      // TCP send buffer is full — wait for it to drain and retry
      int retries = 0;
      while (written == 0 && retries < 20 && client.connected()) {
        delay(10);
        written = client.write(data + offset, chunk);
        retries++;
      }
      if (written == 0) {
        Serial.printf("[WA] ERROR: short write at %u/%u bytes (connected=%d)\n",
                      (unsigned)offset, (unsigned)len, client.connected() ? 1 : 0);
        return;
      }
    }

    offset += written;
    yield();
  }

  client.flush();
}

void WifiAdmin::handleBattery() {
  markHttpActivity();
  _server->sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
  uint8_t pct = System::getBatteryPercentage();
  bool charging = System::isCharging();
  String payload = String("{\"pct\":") + String(pct) +
                   String(",\"charging\":") + (charging ? "true" : "false") + "}";
  _server->send(200, "application/json", payload);
}

void WifiAdmin::handleUpsertAccount() {
  markHttpActivity();
  int idx = _server->arg("index").toInt();
  Serial.printf("[WA] POST /accounts  idx=%d\n", idx);
  if (idx < 0 || idx >= Storage::maxSlots()) {
    Serial.printf("[WA] upsert rejected: invalid index %d\n", idx);
    _server->sendHeader("Location", "/accounts?status=bad_index", true);
    _server->send(302, "text/plain", "");
    return;
  }

  uint8_t slotCount = _storage->getSlotCount();
  Credentials creds;
  if (idx < slotCount) {
    creds = _storage->read(idx, _userPin);
  } else {
    memset(&creds, 0, sizeof(creds));
  }

  String passwordArg = _server->arg("password");
  _server->arg("name")    .toCharArray(creds.name,     sizeof(creds.name));
  _server->arg("username").toCharArray(creds.username, sizeof(creds.username));
  if (passwordArg.length() > 0) {
    passwordArg.toCharArray(creds.password, sizeof(creds.password));
    Serial.println("[WA] password updated");
  } else {
    Serial.println("[WA] password unchanged");
  }

  _storage->store(idx, creds, _userPin);
  EEPROM.commit();
  Serial.printf("[WA] slot %d saved\n", idx);
  _server->sendHeader("Location", "/accounts?status=saved", true);
  _server->send(302, "text/plain", "");
}

void WifiAdmin::handleDeleteAccountByPath() {
  markHttpActivity();
  int idx = parseAccountIndexFromPath(_server->uri());
  Serial.printf("[WA] DELETE /accounts/%d\n", idx);
  if (idx < 0) {
    Serial.println("[WA] delete rejected: invalid path");
    _server->send(400, "text/plain", "Invalid index");
    return;
  }

  if (idx >= _storage->getSlotCount()) {
    Serial.printf("[WA] delete rejected: slot %d not found (count=%d)\n", idx, _storage->getSlotCount());
    _server->send(404, "text/plain", "Account not found");
    return;
  }

  _storage->deleteSlot(idx);
  EEPROM.commit();
  Serial.printf("[WA] slot %d deleted\n", idx);
  _server->send(204, "text/plain", "");
}

void WifiAdmin::handleChangePin() {
  markHttpActivity();
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
    Serial.println("[WA] change-pin rejected: new PIN mismatch");
    _server->sendHeader("Location", "/settings?status=pin_mismatch", true);
    _server->send(302, "text/plain", "");
    return;
  }
  if (!_storage->resetPin(current, next)) {
    Serial.println("[WA] change-pin rejected: current PIN wrong");
    _server->sendHeader("Location", "/settings?status=pin_wrong", true);
    _server->send(302, "text/plain", "");
    return;
  }
  memcpy(_userPin, next, 4);
  Serial.println("[WA] PIN changed successfully");
  _server->sendHeader("Location", "/settings?status=pin_ok", true);
  _server->send(302, "text/plain", "");
}

void WifiAdmin::handleResetWifiPass() {
  markHttpActivity();
  Serial.println("[WA] resetting WiFi password");
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

int WifiAdmin::parseAccountIndexFromPath(const String& uri) const {
  const String prefix = "/accounts/";
  if (!uri.startsWith(prefix)) return -1;

  String tail = uri.substring(prefix.length());
  if (tail.length() == 0) return -1;
  for (size_t i = 0; i < tail.length(); i++) {
    char c = tail.charAt(i);
    if (c < '0' || c > '9') return -1;
  }

  int idx = tail.toInt();
  if (idx < 0 || idx >= Storage::maxSlots()) return -1;
  return idx;
}

String WifiAdmin::buildHead(const char* activeTab) {
  String html;
  html.reserve(4096);
  html =
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<link rel='icon' href='/favicon.svg' type='image/svg+xml'>"
    "<link rel='alternate icon' href='/favicon.ico'>"
    "<title>ESPWV32 Admin</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:560px;margin:24px auto;padding:0 12px;background:#f4f4f4}"
    "h1{font-size:1.4em;color:#333}"
    "h2{font-size:1em;color:#555;margin:0 0 8px}"
    "p{margin:6px 0}"
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
    ".warn{color:#c00;font-weight:bold}"
    ".hint{color:#666;font-size:.85em}"
    ".nav{display:flex;align-items:center;gap:8px;margin:10px 0 14px}"
    ".tab{display:inline-block;text-decoration:none;padding:6px 10px;border-radius:4px;background:#e9ecef;color:#333}"
    ".tab.active{background:#0069d9;color:#fff}"
    ".spacer{flex:1}"
    ".topstat{font-size:.85em;color:#555;white-space:nowrap}"
    ".bat-row{display:flex;align-items:center;gap:8px;flex-wrap:wrap}"
    ".battery-shell{position:relative;width:42px;height:18px;border:2px solid #666;border-radius:4px;background:#f2f2f2;overflow:hidden}"
    ".battery-cap{width:4px;height:10px;background:#666;border-radius:0 2px 2px 0;margin-left:-6px}"
    ".battery-fill{height:100%;width:0%;background:#f0ad4e;transition:width .25s ease,background-color .25s ease}"
    ".battery-fill.full{background:#29a329}"
    ".battery-fill.used{background:#f0ad4e}"
    ".battery-fill.low{background:#d9534f}"
    ".charging{color:#1f6feb;font-weight:bold}"
    "</style>"
    "<script>"
    "function batteryState(p){if(p<=20)return 'Almost empty';if(p>=80)return 'Full';return 'Used';}"
    "function batteryClass(p){if(p<=20)return 'low';if(p>=80)return 'full';return 'used';}"
    "function deleteAccount(idx){if(!confirm('Delete slot '+idx+'?'))return;fetch('/accounts/'+idx,{method:'DELETE'}).then(function(r){if(!r.ok)throw new Error('delete failed');window.location.reload();}).catch(function(){alert('Delete failed');});}"
    "function updateBattery(){fetch('/battery',{cache:'no-store'}).then(function(r){return r.json();}).then(function(d){"
    "var p=Math.max(0,Math.min(100,parseInt(d.pct||0,10)));"
    "var tp=document.getElementById('top-battery');if(tp)tp.textContent=p+'%';"
    "var tc=document.getElementById('top-charge');if(tc)tc.textContent=d.charging?' ⚡':'';"
    "var pct=document.getElementById('battery-pct');if(pct)pct.textContent=p;"
    "var state=document.getElementById('battery-state');if(state)state.textContent=batteryState(p);"
    "var fill=document.getElementById('battery-fill');if(fill){fill.style.width=p+'%';fill.className='battery-fill '+batteryClass(p);}"
    "var ch=document.getElementById('charging-ind');if(ch){ch.innerHTML=d.charging?'&#9889; Charging':'Not charging';ch.className=d.charging?'charging':'';}"
    "}).catch(function(){});}"
    "setInterval(updateBattery,1000);"
    "window.addEventListener('load',updateBattery);"
    "</script></head><body>"
    "<h1>&#x1F512; ESPWV32 Account Manager</h1>";

  html += "<div class='nav'>"
          "<a class='tab " + String(strcmp(activeTab, "accounts") == 0 ? "active" : "") + "' href='/accounts'>Accounts</a>"
          "<a class='tab " + String(strcmp(activeTab, "settings") == 0 ? "active" : "") + "' href='/settings'>Settings</a>"
          "<div class='spacer'></div>"
          "<span class='topstat' id='top-battery'>--%</span>"
          "<span class='topstat' id='top-charge'></span>"
          "</div>";

  return html;
}

String WifiAdmin::buildFooter() {
  return "</body></html>";
}

String WifiAdmin::buildAccountsPage(String status, int selectedSlot) {
  String html;
  html.reserve(4096);
  html =
    "<!DOCTYPE html><html><head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<link rel='icon' href='/favicon.svg' type='image/svg+xml'>"
    "<link rel='alternate icon' href='/favicon.ico'>"
    "<title>ESPWV32 Accounts</title>"
    "<style>"
    "body{font-family:sans-serif;max-width:560px;margin:24px auto;padding:0 12px;background:#f4f4f4}"
    "h1{font-size:1.4em;color:#333}"
    "h2{font-size:1em;color:#555;margin:0 0 8px}"
    "p{margin:6px 0}"
    ".card{background:#fff;border-radius:6px;padding:12px;margin:10px 0;box-shadow:0 1px 3px rgba(0,0,0,.15)}"
    "label{display:block;font-size:.8em;color:#666;margin-top:6px}"
    "input[type=text],input[type=password]{width:100%;padding:6px;box-sizing:border-box;border:1px solid #ccc;border-radius:3px}"
    "button{margin-top:8px;background:#0069d9;color:#fff;border:none;padding:7px 18px;border-radius:3px;cursor:pointer}"
    "button.del{background:#c00;margin-left:6px}"
    ".btn-row{display:flex;align-items:center}"
    ".ok{color:#1a7f1a;font-weight:bold;margin-bottom:8px}"
    ".err{color:#c00;font-weight:bold;margin-bottom:8px}"
    ".warn{color:#c00;font-weight:bold}"
    ".hint{color:#666;font-size:.85em}"
    ".nav{display:flex;align-items:center;gap:8px;margin:10px 0 14px}"
    ".tab{display:inline-block;text-decoration:none;padding:6px 10px;border-radius:4px;background:#e9ecef;color:#333}"
    ".tab.active{background:#0069d9;color:#fff}"
    "</style>"
    "<script>"
    "function deleteAccount(idx){if(!confirm('Delete slot '+idx+'?'))return;fetch('/accounts/'+idx,{method:'DELETE'}).then(function(r){if(!r.ok)throw new Error('delete failed');window.location.reload();}).catch(function(){alert('Delete failed');});}"
    "</script></head><body>"
    "<h1>&#x1F512; ESPWV32 Account Manager</h1>"
    "<div class='nav'>"
    "<a class='tab active' href='/accounts'>Accounts</a>"
    "<a class='tab' href='/settings'>Settings</a>"
    "</div>";

  if (status == "saved") html += "<p class='ok'>&#10003; Account saved.</p>";
  else if (status == "bad_index") html += "<p class='err'>&#10007; Invalid account slot.</p>";

  uint8_t slotCount = _storage->getSlotCount();
  int maxSlots = Storage::maxSlots();
  int remainingSlots = maxSlots - (int)slotCount;
  bool lowRemaining = remainingSlots <= 2;
  int editableCount = min((int)slotCount + 1, maxSlots);
  if (editableCount <= 0) {
    html += "<div class='card'><p class='err'>&#10007; No available slots.</p></div>";
    html += buildFooter();
    return html;
  }
  int editIdx = (selectedSlot >= 0) ? selectedSlot : ((slotCount > 0) ? 0 : 0);
  if (editIdx < 0) editIdx = 0;
  if (editIdx >= editableCount) editIdx = editableCount - 1;

  html += "<div class='card'><h2>Slots</h2><p>Used: <b>" + String(slotCount) +
          "</b> / Max: <b>" + String(maxSlots) + "</b><br>Remaining: <span class='" +
          String(lowRemaining ? "warn" : "") + "'><b>" + String(remainingSlots) + "</b></span></p></div>";

  html += "<div class='card'><h2>Select Slot</h2><p>";
  for (int i = 0; i < editableCount; i++) {
    String label = (i < slotCount) ? ("Slot " + String(i)) : "+ New";
    html += "<a class='tab " + String(i == editIdx ? "active" : "") + "' href='/accounts?slot=" + String(i) + "'>" + label + "</a> ";
  }
  html += "</p></div>";

  Credentials c;
  if (editIdx < slotCount) c = _storage->read(editIdx, _userPin);
  else memset(&c, 0, sizeof(c));

  html += "<div class='card'>"
          "<form method='POST' action='/accounts'>"
          "<input type='hidden' name='index' value='" + String(editIdx) + "'>"
          "<b>" + String(editIdx < slotCount ? "Edit slot " : "Create slot ") + String(editIdx) + "</b>"
          "<label>Name</label>"
          "<input type='text' name='name' value='" + esc(c.name) + "'>"
          "<label>Username</label>"
          "<input type='text' name='username' value='" + esc(c.username) + "'>"
          "<label>Password</label>"
          "<input type='password' name='password' value='' placeholder='" +
          String(editIdx < slotCount ? "Leave blank to keep current password" : "Enter password") + "'>"
          "<p class='hint'>Existing passwords are never shown.</p>"
          "<div class='btn-row'>"
          "<button type='submit'>Save</button>";

  if (editIdx < slotCount) {
    html += "<button class='del' type='button' onclick='deleteAccount(" + String(editIdx) + ")'>Delete</button>";
  }

  html += "</div></form></div>";

  html += buildFooter();
  return html;
}

String WifiAdmin::buildSettingsPage(String status) {
  String html = buildHead("settings");

  uint8_t initialPct = System::getBatteryPercentage();
  bool initialCharging = System::isCharging();
  const char* initialState = (initialPct <= 20) ? "Almost empty" : ((initialPct >= 80) ? "Full" : "Used");
  const char* initialClass = (initialPct <= 20) ? "low" : ((initialPct >= 80) ? "full" : "used");

  html += "<div class='card'><h2>Battery</h2><div class='bat-row'>"
          "<div class='battery-shell'><div id='battery-fill' class='battery-fill " + String(initialClass) +
          "' style='width:" + String(initialPct) + "%'></div></div>"
          "<div class='battery-cap'></div>"
          "<b><span id='battery-pct'>" + String(initialPct) + "</span>%</b>"
          "<span id='battery-state'>" + String(initialState) + "</span>"
          "<span id='charging-ind'" + String(initialCharging ? " class='charging'" : "") + ">" +
          String(initialCharging ? "&#9889; Charging" : "Not charging") +
          "</span>"
          "</div></div>";

  if      (status == "pin_ok")       html += "<p class='ok'>&#10003; PIN changed.</p>";
  else if (status == "pin_wrong")    html += "<p class='err'>&#10007; Current PIN incorrect.</p>";
  else if (status == "pin_mismatch") html += "<p class='err'>&#10007; New PINs do not match.</p>";

  html += "<div class='card'><h2>&#x1F4F6; WiFi Admin Password</h2>"
          "<p style='font-size:.9em;color:#555;margin:4px 0'>SSID: <b>" WIFI_ADMIN_SSID "</b></p>"
          "<form method='POST' action='/reset-wifi-pass'>"
          "<button type='submit' "
          "onclick=\"return confirm('Generate a new WiFi password? You will need to reconnect.')\""
          ">Generate new password</button>"
          "</form></div>";

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

  uint8_t slotCount = _storage->getSlotCount();
  int maxSlots = Storage::maxSlots();
  int remainingSlots = maxSlots - (int)slotCount;
  bool lowRemaining = remainingSlots <= 2;
  html += "<div class='card'><h2>Slots</h2><p style='margin:4px 0'>Used: <b>" +
          String(slotCount) + "</b> / Max: <b>" + String(maxSlots) +
          "</b><br>Remaining: <span class='" + String(lowRemaining ? "warn" : "") +
          "'><b>" + String(remainingSlots) + "</b></span></p></div>";

  html += buildFooter();
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

