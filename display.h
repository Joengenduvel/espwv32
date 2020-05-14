#include <Arduino.h>
#include <M5StickC.h>

namespace display {
class Display {
  public:
    void showStart(String deviceID);
    void showPin(uint32_t pin);
    void showInputPin(uint8_t userPin[], uint8_t index);
    void updateBatteryPercentage(uint8_t percentage);
    void updateConnected(bool connected);

};
}
