#include "GenericScreen.h"
#include <string.h>

namespace espwv32 {

class SerialAdminScreen : public GenericScreen {
  public:
    SerialAdminScreen() {
      memset(_userPin, 0, 4);
    }

    void updatePin(uint8_t* pin) {
      memcpy(_userPin, pin, 4);
      reset();
    }

    uint8_t* getPin() { return _userPin; }

    ScreenType getType() { return SERIAL_ADMIN; }

    void buttonPressedA() override { _toNextScreen = true; }
    void buttonMediumPressedA() override { _toNextScreen = true; }
    void buttonLongPressedA() override { _toNextScreen = true; }

  private:
    uint8_t _userPin[4];

    void show() override {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);

      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(CYAN);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.print("Serial Admin");

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setCursor(0, 18);
      M5.Lcd.print("USB serial: 115200 8N1");
      M5.Lcd.setCursor(0, 28);
      M5.Lcd.print("Windows: COMx");
      M5.Lcd.setCursor(0, 38);
      M5.Lcd.print("macOS: /dev/cu.usb*");
      M5.Lcd.setCursor(0, 48);
      M5.Lcd.print("Linux: /dev/ttyUSB0");
      M5.Lcd.setCursor(0, 58);
      M5.Lcd.print("Type HELP in terminal");

      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.setCursor(0, 70);
      M5.Lcd.print("[A] Back to Accounts");

      Serial.println("[SA] Serial admin info screen opened");
      Serial.println("[SA] Connect at 115200 8N1 and type HELP");
    }
};

} // namespace espwv32


