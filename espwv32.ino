#include <M5StickC.h>
#include "ble.h"
#include "GenericScreen.h"
#include "StartScreen.cpp"
#include "PinScreen.cpp"
#include "LockScreen.cpp"
#include "AccountSelectionScreen.cpp"
#include "Storage.h"

espwv32::GenericScreen* _currentScreen;
espwv32::GenericScreen* _startScreen;
espwv32::GenericScreen* _pinScreen;
espwv32::GenericScreen* _lockScreen;
espwv32::GenericScreen* _accountSelectionScreen;


ble::BLEKeyboard* _keyboard;

espwv32::Credentials storedCredentials[] = {
  {
    "Account 1",
    "User 1",
    "Password 1"
  },
  {
    "Account 2",
    "User 2",
    "Password 2"
  },
  {
    "Account 3",
    "User 3",
    "Password 3"
  },
  {
    "Account 4",
    "User 4",
    "Password 4"
  }
};

class MyKeyboardCallbacks: public ble::BLEKeyboardCallbacks {
    void authenticationInfo(uint32_t pin) {
      Serial.println(pin);
      delete _pinScreen;
      _pinScreen = new espwv32::PinScreen(pin);
      _currentScreen = _pinScreen;
    }

    void connected() {
      Serial.println("connected");
      delete _lockScreen;
      _lockScreen = new espwv32::LockScreen();
      _currentScreen = _lockScreen;
    }
    void disconnected() {
      Serial.println("disconnected");
      _currentScreen = _startScreen;
      _currentScreen->reset();
    }
};

void setup() {
  //setCpuFrequencyMhz(80);

  Serial.begin(115200);
  M5.begin();
  EEPROM.begin(1000);

  _startScreen = new espwv32::StartScreen("");
  _startScreen->next();
  _currentScreen = _startScreen;

  _keyboard = new ble::BLEKeyboard("");
  _keyboard->setCallbacks(new MyKeyboardCallbacks());

  espwv32::Storage* storage = new espwv32::Storage();

  //Initialising accounts until this feature is implemented
  for (byte index = 0; index < sizeof(storedCredentials) / sizeof(espwv32::Credentials); index++) {
    espwv32::Credentials credsR = storage->read(index);
    Serial.printf("Updating %d = %s\n", index, credsR.name);
    if (!String(storedCredentials[index].name).equals(String(credsR.name))) {
      Serial.printf("Store %d = %s \n", index, storedCredentials[index].name);
      storage->store(index, storedCredentials[index]);
    } else {
      Serial.printf("Verified %d = %s == %s \n", index, credsR.name, storedCredentials[index].name);
    }
  }
  Serial.println(EEPROM.commit());
}



void loop() {
  M5.update();
  if (_currentScreen->next()) {
    delete _currentScreen;
    switch (_currentScreen->getType()) {
      case espwv32::ScreenType::LOCK:
        _accountSelectionScreen = new espwv32::AccountSelectionScreen(_keyboard);
        _currentScreen = _accountSelectionScreen;
        break;
      default:
        Serial.println("unknown type");

    }
  }


  if (millis() % 77 == 0) {
    //keyboard->setBatteryLevel(getBatteryPercentage());
  }

  if (M5.BtnA.wasPressed()) {
    _currentScreen->buttonPressedA();
  }

  if (M5.BtnA.pressedFor(1000)) {
    _currentScreen->buttonMediumPressedA();
  }

  if (M5.BtnA.pressedFor(2000)) {
    _currentScreen->buttonLongPressedA();
  }

  if (M5.BtnB.wasPressed()) {
    _currentScreen->buttonPressedB();
  }

  if (M5.BtnB.pressedFor(1000)) {
    _currentScreen->buttonMediumPressedB();
  }

  if (M5.BtnB.pressedFor(2000)) {
    _currentScreen->buttonLongPressedB();
  }

  if (M5.BtnA.pressedFor(3000)) {
    //M5.Axp.DeepSleep(SLEEP_SEC(10));
    //keyboard->disconnect();
    Serial.println("Going to sleep");

  }

  //
}
