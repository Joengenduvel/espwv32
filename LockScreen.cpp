#include "GenericScreen.h"

#define numberOfDigits 4

namespace espwv32 {

class LockScreen : public GenericScreen {
    void buttonPressedA() {
      _userPinIndex++;
      if (_userPinIndex >= numberOfDigits)
        _userPinIndex = 0;
      show();
    }
    void buttonMediumPressedA() {
      M5.Lcd.fillScreen(BLACK);
      _toNextScreen = true;
    }
    void buttonPressedB() {
      _userPin[_userPinIndex]++;
      if (_userPin[_userPinIndex] > 9)
        _userPin[_userPinIndex] = 0;
      show();
    }

    void reset() {
      GenericScreen::reset();
      for (uint8_t i = 0; i < numberOfDigits; i++) {
        _userPin[i] = 0;
      }
    }
  public:
    LockScreen() {
      reset();
    }

    ScreenType getType() {
      return LOCK;
    }
    
    uint8_t* getCode(){      
      Serial.printf("Obtaining pin %d%d%d%d\n", _userPin[0], _userPin[1], _userPin[2], _userPin[3]);
      return _userPin;
    }
  private:
    uint8_t _userPin[numberOfDigits];
    uint8_t _userPinIndex = 0;
    void show() {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextSize(5);
      M5.Lcd.setCursor(0, 20);
      for (uint8_t i = 0; i < numberOfDigits; i++) {
        if (i == _userPinIndex) {

          M5.Lcd.setTextColor(RED);
        } else {
          M5.Lcd.setTextColor(WHITE);
        }
        M5.Lcd.print(_userPin[i]);
      }
    }
};
}
