#include <EEPROM.h>
#include <hwcrypto/aes.h>
#include <Arduino.h>  // for type definitions

#ifndef MY_CLASS_Storage // include guard
#define MY_CLASS_Storage

namespace espwv32 {
//must be n times 16 to be able to be encrypted in this case 6*16 = 96
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
    bool store(byte index, Credentials creds);
    Credentials read(byte index);
  private:
    Credentials encrypt(Credentials credentials);
    Credentials decrypt(Credentials credentials);
};

}
#endif /* MY_CLASS_Storage */
