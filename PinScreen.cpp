#include "GenericScreen.h"

namespace espwv32 {

class PinScreen : public GenericScreen {
    
  public:
    PinScreen(uint32_t pin) {
      _pin = pin;
      reset();
    }
    ScreenType getType(){
      return PIN;
    }
  private:
    uint32_t _pin;
    void show() {
      M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.print("PIN:");
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.setTextSize(3);
  M5.Lcd.print(_pin);
    }
};
}
