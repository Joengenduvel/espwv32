#include <EEPROM.h>
#include <hwcrypto/aes.h>
#include <Arduino.h>  // for type definitions

#ifndef MY_CLASS_Storage // include guard
#define MY_CLASS_Storage

namespace espwv32 {
//must be n times 16 to be able to be encrypted in this case 6*16 = 96 = 26 + 38 + 32
struct Credentials {
  char name[26];
  char username[38];
  char password[32];
};

class Storage {
  public:
    Storage() {
      //EEPROM.begin(4096);
    }
    ~Storage() {
      //EEPROM.end();
    }
    bool store(byte index, Credentials creds, uint8_t pin[]);
    Credentials read(byte index, uint8_t pin[]);
  private:
    Credentials encrypt(Credentials credentials, uint8_t pin[]);
    Credentials decrypt(Credentials credentials, uint8_t pin[]);
    uint8_t* calculateIV(uint8_t iv[],uint8_t* pin);
    uint8_t* calculateKey(uint8_t key[],uint8_t* pin);
};

}
#endif /* MY_CLASS_Storage */
