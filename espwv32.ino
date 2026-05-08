#include <M5StickC.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include "ble.h"
#pragma GCC diagnostic pop
#include "GenericScreen.h"
#include "StartScreen.cpp"
#include "PinScreen.cpp"
#include "LockScreen.cpp"
#include "AccountSelectionScreen.cpp"
#include "WifiAdminScreen.cpp"
#include "Storage.h"
#include "System.cpp"
#include "esp_bt.h"



espwv32::GenericScreen* _currentScreen;
espwv32::GenericScreen* _startScreen;
espwv32::GenericScreen* _pinScreen;
espwv32::GenericScreen* _lockScreen;
espwv32::GenericScreen* _accountSelectionScreen;
espwv32::GenericScreen* _wifiAdminScreen;



class MyKeyboardCallbacks: public ble::BLEKeyboardCallbacks {
    void authenticationInfo(uint32_t pin) {
      Serial.println(pin);
      ((espwv32::PinScreen*)_pinScreen)->updatePin(pin);
      _currentScreen = _pinScreen;
    }

    void connected() {
      Serial.println("connected");
      espwv32::Storage storage;
      espwv32::LockScreen* lock = (espwv32::LockScreen*)_lockScreen;
      lock->setMode(storage.isPinConfigured()
        ? espwv32::LockScreen::ENTER
        : espwv32::LockScreen::SET);
      lock->reset();
      _currentScreen = _lockScreen;
    }

    void disconnected() {
      Serial.println("disconnected");
      // Ignore BLE disconnect while in WiFi admin — BLE advertising is
      // stopped but an existing connection is kept alive; if it drops
      // naturally we stay in admin mode rather than jumping to Start Screen
      if (_currentScreen->getType() == espwv32::ScreenType::WIFI_ADMIN) return;
      _currentScreen = _startScreen;
      _currentScreen->reset();
    }
};

void setup() {
  setCpuFrequencyMhz(80);

  Serial.begin(115200);
  M5.begin();
  M5.Axp.ScreenBreath(15); // 7=off, 15=max brightness
  // Probe the largest EEPROM size the library accepts, halving until it works.
  // On ESP32 the Arduino EEPROM library is capped at 4096 bytes (one flash page).
  int eepromSize = 4096;
  while (eepromSize >= (int)sizeof(espwv32::Credentials) + 2) {
    if (EEPROM.begin(eepromSize)) break;
    eepromSize /= 2;
  }
  espwv32::Storage::setSize(eepromSize);
  Serial.printf("EEPROM: %d bytes → max slots: %d\n", eepromSize, espwv32::Storage::maxSlots());

  // Pre-initialise the WiFi stack before BLE to ensure the esp-netif
  // layer is ready before BLE claims heap.
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);

  // Release Classic BT controller memory — this project uses BLE only.
  // Classic BT reserves ~70 KB by default; freeing it gives lwIP enough
  // heap to allocate packet buffers when WiFi AP and BLE coexist.
  // Must be called BEFORE BLEDevice::init() (inside BLEKeyboard ctor).
  esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);

  _startScreen            = new espwv32::StartScreen("");
  _pinScreen              = new espwv32::PinScreen();
  _lockScreen             = new espwv32::LockScreen();
  _accountSelectionScreen = new espwv32::AccountSelectionScreen(); // owns the BLEKeyboard
  _wifiAdminScreen        = new espwv32::WifiAdminScreen();

  // BLE-only mode at boot — give BLE full radio priority
  esp_coex_preference_set(ESP_COEX_PREFER_BT);

  _startScreen->reset();
  _currentScreen = _startScreen;

  ((espwv32::AccountSelectionScreen*)_accountSelectionScreen)->setCallbacks(new MyKeyboardCallbacks());}

void loop() {
  M5.update();
  _currentScreen->handle();

  if (_currentScreen->next()) {
    switch (_currentScreen->getType()) {
      case espwv32::ScreenType::SET_PIN:
        {
          // First-time PIN set — go straight to WiFi Admin to load accounts
          uint8_t* pin = ((espwv32::LockScreen*)_lockScreen)->getCode();
          ((espwv32::WifiAdminScreen*)_wifiAdminScreen)->updatePin(pin);
          _currentScreen = _wifiAdminScreen;
        }
        break;
      case espwv32::ScreenType::LOCK:
        {
          uint8_t* userPin = ((espwv32::LockScreen*)_lockScreen)->getCode();
          espwv32::Storage storage;
          if (storage.getSlotCount() > 0) {
            ((espwv32::AccountSelectionScreen*)_accountSelectionScreen)->updatePin(userPin);
            _currentScreen = _accountSelectionScreen;
          } else {
            // No accounts yet — guide the user to WiFi Admin to add some
            ((espwv32::WifiAdminScreen*)_wifiAdminScreen)->updatePin(userPin);
            _currentScreen = _wifiAdminScreen;
          }
        }
        break;
      case espwv32::ScreenType::ACCOUNT_SELECTION:
        {
          uint8_t* pin = ((espwv32::AccountSelectionScreen*)_accountSelectionScreen)->getPin();
          ((espwv32::WifiAdminScreen*)_wifiAdminScreen)->updatePin(pin);
          _currentScreen = _wifiAdminScreen;
        }
        break;
      case espwv32::ScreenType::WIFI_ADMIN:
        {
          _accountSelectionScreen->reset();
          _currentScreen = _accountSelectionScreen;
        }
        break;
      default:
        Serial.println("unknown type");
    }
  }


  static unsigned long lastStatusUpdate = 0;
  if (millis() - lastStatusUpdate >= 1000) {
    lastStatusUpdate = millis();
    _currentScreen->updateBatteryPercentage(
      espwv32::System::getBatteryPercentage(),
      espwv32::System::isCharging());
  }


  if (M5.BtnA.wasReleasefor(1000)) {
    Serial.println("A long pressed");
    _currentScreen->buttonLongPressedA();
  } else if (M5.BtnA.wasReleasefor(500)) {
    Serial.println("A medium pressed");
    _currentScreen->buttonMediumPressedA();
  } else if (M5.BtnA.wasReleasefor(1)) {
    Serial.println("A pressed");
    _currentScreen->buttonPressedA();
  }


  if (M5.BtnB.wasReleasefor(1000)) {
    Serial.println("B long pressed");
    _currentScreen->buttonLongPressedB();
  } else if (M5.BtnB.wasReleasefor(500)) {
    Serial.println("B medium pressed");
    _currentScreen->buttonMediumPressedB();
  } else if (M5.BtnB.wasReleasefor(1)) {
    Serial.println("B pressed");
    _currentScreen->buttonPressedB();
  }
}
