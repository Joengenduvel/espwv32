#include <M5StickC.h>
#include "ble.h"
#include "GenericScreen.h"
#include "StartScreen.cpp"
#include "PinScreen.cpp"
#include "LockScreen.cpp"
#include "AccountSelectionScreen.cpp"
#include "Storage.h"
#include "System.cpp"



espwv32::GenericScreen* _currentScreen;
espwv32::GenericScreen* _startScreen;
espwv32::GenericScreen* _pinScreen;
espwv32::GenericScreen* _lockScreen;
espwv32::GenericScreen* _accountSelectionScreen;


ble::BLEKeyboard* _keyboard;


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

  storeDummyAccounts();
}



void loop() {
  M5.update();
  if (_currentScreen->next()) {
    switch (_currentScreen->getType()) {
      case espwv32::ScreenType::LOCK:
        {
          uint8_t* userPin = ((espwv32::LockScreen*)_lockScreen)->getCode();
          
      Serial.printf("Initialising Account Selection with pin %d%d%d%d\n", userPin[0], userPin[1], userPin[2], userPin[3]);
          delete _accountSelectionScreen;
          _accountSelectionScreen = new espwv32::AccountSelectionScreen(_keyboard, userPin);
          _currentScreen = _accountSelectionScreen;
        }
        break;
      default:
        Serial.println("unknown type");

    }
  }


  if (millis() % 77 == 0) {
    _currentScreen->updateBatteryPercentage(espwv32::System::getBatteryPercentage());
    _keyboard->setBatteryLevel(espwv32::System::getBatteryPercentage());
    _currentScreen->updateConnected(_keyboard->isConnected());
  }
  if (M5.BtnA.wasReleasefor(1)) {
    Serial.println("A pressed");
    _currentScreen->buttonPressedA();
  }

  if (M5.BtnA.wasReleasefor(1000)) {
    Serial.println("A medium pressed");
    _currentScreen->buttonMediumPressedA();
  }

  if (M5.BtnA.wasReleasefor(2000)) {
    Serial.println("A long pressed");
    _currentScreen->buttonLongPressedA();
  }

  if (M5.BtnA.wasReleasefor(3000)) {
    //M5.Axp.DeepSleep(SLEEP_SEC(10));
    //keyboard->disconnect();
    Serial.println("Going to sleep");

  }

  if (M5.BtnB.wasReleasefor(1)) {
    Serial.println("B pressed");
    _currentScreen->buttonPressedB();
  }

  if (M5.BtnB.wasReleasefor(1000)) {
    Serial.println("B medium pressed");
    _currentScreen->buttonMediumPressedB();
  }

  if (M5.BtnB.wasReleasefor(2000)) {
    Serial.println("B long pressed");
    _currentScreen->buttonLongPressedB();
  }

}

void storeDummyAccounts() {
  uint8_t _userPin[4] = {1, 2, 3, 4};
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
  Serial.println("inserting dummy credentials");
  espwv32::Storage* storage = new espwv32::Storage();
  //Initialising accounts until this feature is implemented
  for (byte index = 0; index < sizeof(storedCredentials) / sizeof(espwv32::Credentials); index++) {
    espwv32::Credentials credsR = storage->read(index, _userPin);
    if (!String(storedCredentials[index].name).equals(String(credsR.name))) {
      Serial.printf("Store %d = %s \n", index, storedCredentials[index].name);
      storage->store(index, storedCredentials[index], _userPin);
    } else {
      Serial.printf("Verified %d = %s == %s \n", index, credsR.name, storedCredentials[index].name);
    }
  }
  Serial.println(EEPROM.commit());
}
