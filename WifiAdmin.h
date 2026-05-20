#ifndef MY_CLASS_WifiAdmin
#define MY_CLASS_WifiAdmin

#include "Storage.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "esp_wifi.h"
#include "esp_coexist.h"
#include "esp_random.h"

#define WIFI_ADMIN_SSID "ESPWV32-Admin"
#define DNS_PORT 53

namespace espwv32 {

// Manages the WiFi Access Point and the web administration interface.
// No BLE dependency — advertising lifecycle is handled by WifiAdminScreen.
class WifiAdmin {
  public:
    WifiAdmin();
    ~WifiAdmin();

    void   updatePin(uint8_t pin[]);
    void   start();                   // start AP + web server
    void   stop();                    // stop AP + web server
    void   handle();                  // call every loop() iteration

    // true (once) when the web UI triggered an exit (e.g. password reset)
    bool isDone()        { return _done; }
    void clearDone()     { _done = false; }

    // true (once) when the WiFi password was just regenerated
    bool passChanged()   { bool v = _passChanged; _passChanged = false; return v; }

    String getWifiPass()  { return _wifiPass; }
    String getIP()        { return WiFi.softAPIP().toString(); }
    uint8_t* getPin()     { return _userPin; }

  private:
    Storage*          _storage     = nullptr;
    WebServer*        _server      = nullptr;
    DNSServer*        _dns         = nullptr;
    uint8_t           _userPin[4];
    String            _wifiPass;
    bool              _done        = false;
    bool              _passChanged = false;
    bool              _eventsRegistered = false;
    unsigned long     _lastHttpActivityMs = 0;

    static String generatePass();
    void markHttpActivity();
    void handleRoot();
    void handleAccountsPage();
    void handleSettingsPage();
    void handleUpsertAccount();
    void handleDeleteAccountByPath();
    void handleChangePin();
    void handleBattery();
    void handleResetWifiPass();
    int parseAccountIndexFromPath(const String& uri) const;
    void sendHtmlPage(const String& html);
    String buildHead(const char* activeTab);
    String buildFooter();
    String buildAccountsPage(String status = "", int selectedSlot = -1);
    String buildSettingsPage(String status = "");
    static String esc(const char* s);
};

} // namespace espwv32
#endif /* MY_CLASS_WifiAdmin */

