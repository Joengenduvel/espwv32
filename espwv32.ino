#include <M5StickC.h>
#include "ble.h"
#include "display.h"

//using namespace ble;

ble::BLEKeyboard* keyboard;
display::Display* screen;

uint32_t pinCode = 0;
#define PAIR_MAX_DEVICES 20
uint8_t pairedDeviceBtAddr[PAIR_MAX_DEVICES][6];

class MyKeyboardCallbacks: public ble::BLEKeyboardCallbacks {
    void authenticationInfo(uint32_t pin) {
      Serial.println(pin);
      screen->showPin(pin);
    }
    
    void connected(){
      Serial.println("connected");
      //screen->showPin(pin);
    }
    void disconnected(){
      Serial.println("disconnected");
      screen->showStart("");
    }
};

void setup() {
  //setCpuFrequencyMhz(80);

  Serial.begin(115200);
  M5.begin();

  screen = new display::Display();

  screen->showStart(getDeviceId());

  keyboard = new ble::BLEKeyboard(getDeviceId().c_str());
  keyboard->setCallbacks(new MyKeyboardCallbacks());
}

String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();

  uint32_t low = chipid % 0xFFFFFFFF;
  uint32_t high = (chipid >> 32) % 0xFFFFFFFF;

  return String(String(high, HEX) + String(low, HEX)).c_str();
}

uint32_t getBatteryVoltage() {
  return (M5.Axp.GetVbatData() * 1.1);
}

uint8_t getBatteryPercentage() {
  return map(getBatteryVoltage(), 3500, 4125, 0, 100);
}

void loop() {
  M5.update();
  screen->updateBatteryPercentage(getBatteryPercentage());

  if (M5.BtnA.wasPressed()) {
    uint8_t i;
    Serial.println("sending ");



    keyboard->print("username\tpassword\n");

  }

  if (M5.BtnA.pressedFor(1000)) {
    M5.Axp.ScreenBreath(9);
  }

  if (M5.BtnA.pressedFor(2000)) {
    M5.Axp.ScreenBreath(7);
  }

  if (M5.BtnA.pressedFor(3000)) {
    //M5.Axp.DeepSleep(SLEEP_SEC(10));
    keyboard->disconnect();
    Serial.println("Going to sleep");

  }

  //Serial.println(getBatteryLevel());
  //Serial.println(keyboard->isConnected());
  keyboard->setBatteryLevel(getBatteryPercentage());
  //delay(33);
}
