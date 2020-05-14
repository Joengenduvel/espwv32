#include <M5StickC.h>
#include "System.cpp"

#ifndef MY_CLASS_GenericScreen // include guard
#define MY_CLASS_GenericScreen

namespace espwv32 {
enum ScreenType { //Because of no rtti
  START,
  PIN,
  LOCK,
  ACCOUNT_SELECTION
};
class GenericScreen {
  public:
    virtual void buttonPressedA() {}
    virtual void buttonMediumPressedA() {}
    virtual void buttonLongPressedA() {}
    virtual void buttonPressedB() {}
    virtual void buttonMediumPressedB() {}
    virtual void buttonLongPressedB() {}
    virtual void show();
    virtual ScreenType getType();
    virtual void reset() {
      _needToRefresh = true;
    }
    virtual boolean next() {
      if (_needToRefresh) {
        show();
        _needToRefresh = false;
      }
      if (millis() % 77 == 0) {
        updateBatteryPercentage(espwv32::System::getBatteryPercentage());
        updateConnected(false);
      }
      return _toNextScreen;
    }
  private:
    bool _needToRefresh = false;
  protected:
    bool _toNextScreen = false;



    void updateBatteryPercentage(uint8_t percentage = 0) {
      uint8_t posX = 129;
      uint8_t posY = 1;

      M5.Lcd.setRotation(3);
      //clear the space
      M5.Lcd.fillRect(posX, posY, 160 - posX, 7, BLACK);
      //lightning logo
      M5.Lcd.drawLine(posX + 5, posY + 0, posX + 3, posY + 3, BLUE);
      M5.Lcd.drawLine(posX + 3, posY + 3, posX + 5, posY + 4, BLUE);
      M5.Lcd.drawLine(posX + 5, posY + 4, posX + 2, posY + 7, BLUE);
      M5.Lcd.drawLine(posX + 2, posY + 7, posX + 2, posY + 5, BLUE);
      M5.Lcd.drawLine(posX + 2, posY + 7, posX + 4, posY + 7, BLUE);

      M5.Lcd.setTextColor(BLUE);
      M5.Lcd.setCursor(posX + 7, posY);
      M5.Lcd.setTextSize(1);
      M5.Lcd.print(percentage);
      M5.Lcd.print("%");
    }

    void updateConnected(bool connected = false) {
      uint8_t posX = 152;
      uint8_t posY = 15;

      M5.Lcd.setRotation(3);
      //lightning logo
      if (connected) {
        M5.Lcd.fillEllipse(posX, posY, 3, 3, BLUE);
      } else {
        M5.Lcd.fillEllipse(posX, posY, 3, 3, BLACK);
      }

      M5.Lcd.drawEllipse(posX, posY, 3, 3, BLUE);

    }
};
}
#endif /* MY_CLASS_GenericScreen */
