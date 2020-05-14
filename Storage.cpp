#include "Storage.h"

using namespace espwv32;

bool Storage::store(byte index, espwv32::Credentials creds){
  //todo: validate index
  EEPROM.put(index * sizeof(creds), creds);
  return true;
}
espwv32::Credentials Storage::read(byte index){
  //todo: validate index
  Credentials creds;
  return EEPROM.get(index * sizeof(creds), creds);
  //return creds;
}
