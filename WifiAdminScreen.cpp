#include "GenericScreen.h"
#include "WifiAdmin.h"

namespace espwv32 {

class WifiAdminScreen : public GenericScreen {
  public:
    WifiAdminScreen() {
      _admin = new WifiAdmin();
    }
    ~WifiAdminScreen() {
      delete _admin;
    }

    void updatePin(uint8_t* pin) {
      _admin->updatePin(pin);
      reset();
    }

    ScreenType getType() { return WIFI_ADMIN; }

    void handle() override {
      _admin->handle();
      if (_admin->passChanged()) drawScreen();
      if (_admin->isDone())      _exitToAccounts();
    }

    void buttonPressedA()       override { _exitToAccounts(); }
    void buttonMediumPressedA() override { _exitToAccounts(); }
    void buttonLongPressedA()   override { _exitToAccounts(); }

  private:
    WifiAdmin* _admin = nullptr;

    void _exitToAccounts() {
      _admin->stop();
      _admin->clearDone();
      _toNextScreen = true;
    }

    void show() override {
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

      _admin->start();
      drawScreen();
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
      M5.Lcd.setCursor(0, 18);
      M5.Lcd.print("SSID: " WIFI_ADMIN_SSID);
      M5.Lcd.setCursor(0, 28);
      M5.Lcd.print("IP:   ");
      M5.Lcd.print(_admin->getIP());

      M5.Lcd.setCursor(0, 38);
      M5.Lcd.print("Pass:");
      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(0, 46);
      M5.Lcd.print(_admin->getWifiPass());

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.setCursor(0, 68);
      M5.Lcd.print("[A] Back");
    }
};
}
