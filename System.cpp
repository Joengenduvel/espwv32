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

      return String(high, HEX) + String(low, HEX);
    }

    static uint32_t getBatteryVoltage() {
      return (M5.Axp.GetVbatData() * 1.1);
    }

    static uint8_t getBatteryPercentage() {
      // AXP192 on M5StickC charges to ~4130 mV in practice, not 4200 mV.
      // constrain() keeps the value within 0-100 regardless of ADC variance.
      return constrain(map(getBatteryVoltage(), 3200, 4130, 0, 100), 0, 100);
    }

    static bool isCharging() {
      return M5.Axp.GetIchargeData() > 0;
    }
};


}

#endif /* MY_CLASS_System*/
