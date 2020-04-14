#include <M5StickC.h>
#include "ble.cpp"


BLEKeyboard* keyboard;
uint32_t pinCode = 0;

class MyKeyboardCallbacks: public BLEKeyboardCallbacks {
    void authenticationInfo(uint32_t pin) {
      Serial.println(pin);
      pinCode = pin;
    }
};

void setup() {
  //setCpuFrequencyMhz(80);

  Serial.begin(115200);
  M5.begin();

  displayStartScreen();

  keyboard = new BLEKeyboard(getDeviceId().c_str());
  keyboard->setCallbacks(new MyKeyboardCallbacks());
}

String getDeviceId() {
  uint64_t chipid = ESP.getEfuseMac();

  uint32_t low = chipid % 0xFFFFFFFF;
  uint32_t high = (chipid >> 32) % 0xFFFFFFFF;

  return String(String(high, HEX) + String(low, HEX)).c_str();
}

uint8_t getBatteryLevel(){
  return map(M5.Axp.GetVapsData(),2500,3555,0,100);
}

void displayStartScreen() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(M5.Axp.GetVapsData());
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print(getDeviceId());
}

void displayPin() {

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print(M5.Axp.GetVapsData());
  M5.Lcd.setCursor(0, 10);
  M5.Lcd.print("Please enter the following PIN:");
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.setTextSize(2);
  M5.Lcd.print(pinCode);
}
void loop() {
  M5.update();

  if (pinCode > 0) {
    displayPin();
  } else {
    displayStartScreen();
  }

  if (M5.BtnA.wasPressed()) {
    Serial.println("sending");
    keyboard->write(3);
  }

  if (M5.BtnA.pressedFor(1000)) {
    M5.Axp.ScreenBreath(9);
  }

  if (M5.BtnA.pressedFor(2000)) {
    M5.Axp.ScreenBreath(7);
  }

  if (M5.BtnA.pressedFor(3000)) {
    //M5.Axp.DeepSleep(SLEEP_SEC(10));
    Serial.println("Going to sleep");

  }

  Serial.println(getBatteryLevel());
  Serial.println(keyboard->isConnected());
  keyboard->setBatteryLevel(getBatteryLevel());
  delay(50);
}
