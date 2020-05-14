#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

#ifndef MY_CLASS_Storage // include guard
#define MY_CLASS_Storage

namespace espwv32 {
  
struct Credentials {
  char name[26];
  char username[42];
  char password[32];
};

class Storage{
  public:
  Storage(){
    //EEPROM.begin(4096);
  }
  ~Storage(){
    //EEPROM.end();
  }
  bool store(byte index, Credentials creds);
  Credentials read(byte index);
};

}
#endif /* MY_CLASS_Storage */
