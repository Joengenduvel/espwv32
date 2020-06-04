#include "GenericScreen.h"
#include "Storage.h"
#include "ble.h"


namespace espwv32 {

class AccountSelectionScreen: public GenericScreen {
  public:
    AccountSelectionScreen(ble::BLEKeyboard* keyboard, uint8_t* userPin) {
      _storage = new Storage();
      _keyboard = keyboard;
      _userPin = userPin;
      _userPinSize = sizeof(_userPin)/sizeof(uint8_t);
      reset();
    }
    ~AccountSelectionScreen() {
      delete _storage;
    }

    void reset() {
      _accountIndex = 0;
      GenericScreen::reset();
    }
    virtual void buttonPressedA() {
      _accountIndex++;
      if (_accountIndex >= NUM_ACCOUNTS)
        _accountIndex = 0;
      Serial.printf("Next %d \n", _accountIndex);
      show();
    }
    virtual void buttonMediumPressedA() {
      if (_accountIndex <= 0)
        _accountIndex = NUM_ACCOUNTS;
      _accountIndex--;
      Serial.printf("Previous %d \n", _accountIndex);
      show();
    }
    virtual void buttonPressedB() {
      M5.Lcd.fillScreen(WHITE);
      Serial.println("sending");
      switch (_dataToSend) {
        case USERNAME_PASSWORD:
          _keyboard->print(_currentAccount.username);
          _keyboard->print("\t");
          _keyboard->println(_currentAccount.password);
          Serial.print(_currentAccount.username);
          Serial.print("\t");
          Serial.println(_currentAccount.password);
          break;
        case USERNAME:
          _keyboard->print(_currentAccount.username);
          Serial.print(_currentAccount.username);
          break;
        case PASSWORD:
          _keyboard->print(_currentAccount.password);
          Serial.print(_currentAccount.password);
          break;
      }
      show();
    }
    virtual void buttonMediumPressedB() {
      switch (_dataToSend) {
        case USERNAME_PASSWORD:
          _dataToSend = USERNAME;
          break;
        case USERNAME:
          _dataToSend = PASSWORD;
          break;
        case PASSWORD:
          _dataToSend = USERNAME_PASSWORD;
          break;
      }
      show();// only update the specific section
    }
    ScreenType getType() {
      return ACCOUNT_SELECTION;
    }
    void show() {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setRotation(3);

      _currentAccount = _storage->read(_accountIndex);

      M5.Lcd.setCursor(2, 2);
      M5.Lcd.setTextSize(3);
      M5.Lcd.setTextColor(BLUE);
      M5.Lcd.printf("%02d", _accountIndex);

      M5.Lcd.setCursor(2, 40);
      M5.Lcd.setTextSize(2);
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.print(_currentAccount.name);

      M5.Lcd.setCursor(60, 4);
      M5.Lcd.setTextSize(1);
      M5.Lcd.setTextColor(RED);
      M5.Lcd.printf("U%cP  U  P", 175); //https://votreportail.com/codes-ascii.htm

      switch (_dataToSend) {
        case USERNAME_PASSWORD:
          M5.Lcd.drawRect(56, 0, 24, 15, RED );
          break;
        case USERNAME:
          M5.Lcd.drawRect(86, 0, 14, 15, RED );
          break;
        case PASSWORD:
          M5.Lcd.drawRect(103, 0, 14, 15, RED );
          break;
      }

      M5.Lcd.setTextSize(2);
      M5.Lcd.setCursor(-1, 25);
      M5.Lcd.printf("%c", 175);
    }
  private:
    uint8_t* _userPin;
    uint8_t _userPinSize;
    const uint8_t NUM_ACCOUNTS = 10;
    Storage* _storage;
    uint8_t _accountIndex = 0;
    enum DataToSend {
      USERNAME_PASSWORD,
      USERNAME,
      PASSWORD
    };
    DataToSend _dataToSend = USERNAME_PASSWORD;
    Credentials _currentAccount;
    ble::BLEKeyboard* _keyboard;
};
}
