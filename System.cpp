#include <Arduino.h>
#include <M5StickC.h>

#ifndef MY_CLASS_System // include guard
#define MY_CLASS_System

namespace espwv32 {
class System {
  public:
    static String getDeviceId() {
      uint64_t chipid = ESP.getEfuseMac();

      uint32_t low = chipid % 0xFFFFFFFF;
      uint32_t high = (chipid >> 32) % 0xFFFFFFFF;

      return String(String(high, HEX) + String(low, HEX)).c_str();
    }

    static uint32_t getBatteryVoltage() {
      return (M5.Axp.GetVbatData() * 1.1);
    }

    static uint8_t getBatteryPercentage() {
      return map(getBatteryVoltage(), 3200, 4200, 0, 100);
    }
};


}

#endif /* MY_CLASS_System*/
