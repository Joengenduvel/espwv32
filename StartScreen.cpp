#include "GenericScreen.h"

namespace espwv32 {

class StartScreen : public GenericScreen {
  public:
    StartScreen(String deviceId) {
      _deviceId = deviceId;
      reset();
    }
    
    ScreenType getType(){
      return START;
    }
  private:
    String _deviceId;
    void show() {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.setRotation(3);
      M5.Lcd.setTextSize(4);
      M5.Lcd.setCursor(0, 40);
      M5.Lcd.print("ES PWV");

      M5.Lcd.setTextSize(3);
      M5.Lcd.setCursor(90, 10);
      M5.Lcd.print("32");

      //lock body
      M5.Lcd.drawRect(65, 35, 85, 40, RED );
      //lock shackle
      M5.Lcd.drawLine(75, 35, 75, 10, RED);
      M5.Lcd.drawLine(80, 35, 80, 10, RED);
      M5.Lcd.drawLine(140, 35, 140, 10, RED);
      M5.Lcd.drawLine(135, 35, 135, 10, RED);

      M5.Lcd.drawLine(75, 10, 85, 0, RED);
      M5.Lcd.drawLine(80, 10, 90, 0, RED);
      M5.Lcd.drawLine(140, 10, 130, 0, RED);
      M5.Lcd.drawLine(135, 10, 125, 0, RED);

      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(0, 10);
      M5.Lcd.print(_deviceId);
    }
};
}
