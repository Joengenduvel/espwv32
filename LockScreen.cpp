#include "GenericScreen.h"
#include "Storage.h"

#define PIN_DIGITS 4

namespace espwv32 {

class LockScreen : public GenericScreen {
  public:
    enum Mode { ENTER, SET };

    LockScreen() { reset(); }

    void setMode(Mode mode) { _mode = mode; }

    ScreenType getType() {
      // Return SET_PIN when in SET mode so the main loop can route correctly
      return (_mode == SET) ? SET_PIN : LOCK;
    }

    uint8_t* getCode() { return _pin; }

    void reset() override {
      GenericScreen::reset();
      _confirming = false;
      _pinIndex   = 0;
      memset(_pin,        0, PIN_DIGITS);
      memset(_confirmPin, 0, PIN_DIGITS);
    }

    void buttonPressedA() override {
      _pinIndex = (_pinIndex + 1) % PIN_DIGITS;
      show();
    }

    void buttonPressedB() override {
      uint8_t* active = (_mode == SET && _confirming) ? _confirmPin : _pin;
      active[_pinIndex] = (active[_pinIndex] + 1) % 10;
      show();
    }

    void buttonMediumPressedA() override {
      if (_mode == ENTER) {
        _toNextScreen = true;
      } else {
        if (!_confirming) {
          // Move to confirmation phase
          _confirming = true;
          _pinIndex   = 0;
          memset(_confirmPin, 0, PIN_DIGITS);
          show();
        } else {
          if (memcmp(_pin, _confirmPin, PIN_DIGITS) == 0) {
            Storage storage;
            storage.setPinConfigured(_pin);
            _toNextScreen = true;
          } else {
            showError();
            delay(1500);
            reset();
            show();
          }
        }
      }
    }

  private:
    Mode    _mode       = ENTER;
    bool    _confirming = false;
    uint8_t _pin[PIN_DIGITS];
    uint8_t _confirmPin[PIN_DIGITS];
    uint8_t _pinIndex   = 0;

    void show() override {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(YELLOW);
      M5.Lcd.setCursor(0, 5);
      if (_mode == ENTER) {
        M5.Lcd.print("Enter PIN:");
      } else {
        M5.Lcd.print(_confirming ? "Confirm new PIN:" : "Set new PIN:");
      }

      uint8_t* active = (_mode == SET && _confirming) ? _confirmPin : _pin;
      M5.Lcd.setTextSize(5);
      M5.Lcd.setCursor(0, 20);
      for (uint8_t i = 0; i < PIN_DIGITS; i++) {
        M5.Lcd.setTextColor(i == _pinIndex ? RED : WHITE);
        M5.Lcd.print(active[i]);
      }

      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(DARKGREY);
      M5.Lcd.setCursor(0, 60);
      M5.Lcd.print("[A]cursor  [B]+  [A~]ok");
    }

    void showError() {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.setCursor(0, 20);
      M5.Lcd.print("PIN mismatch!");
      M5.Lcd.setCursor(0, 42);
      M5.Lcd.print("Try again...");
    }
};
}
