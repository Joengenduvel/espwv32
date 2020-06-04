#include "Storage.h"

using namespace espwv32;

bool Storage::store(byte index, espwv32::Credentials credentials) {
  //todo: validate index
  EEPROM.put(index * sizeof(credentials), encrypt(credentials));
  return true;
}
espwv32::Credentials Storage::read(byte index) {
  //todo: validate index
  Credentials credentials;
  return decrypt(EEPROM.get(index * sizeof(credentials), credentials));
}

// Clear password
//void clearpass(){
//  memset((uint8_t*) password, 0, PASSLEN ); // Clear
//}

// Crypts password into EEPROM and clears password
espwv32::Credentials Storage::encrypt(espwv32::Credentials credentials) {
  uint8_t key[32];
  uint8_t iv[16];

  esp_aes_context ctx;
  memset( iv, 0, sizeof( iv ) ); //TODO: generate correct Initialisation vector
  memset( key, 0, sizeof( key ) ); //TODO: generate key based on PIN
  esp_aes_init( &ctx );
  esp_aes_setkey( &ctx, key, 256 );

  Credentials encryptedCredentials;
  int result = esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, sizeof(credentials), iv, (uint8_t*) &credentials, (uint8_t*) &encryptedCredentials );
  esp_aes_free( &ctx );
  //TODO: error handling -> result != 0
  //TODO: clean memory
  return encryptedCredentials;
}

// Uncrypts password from EEPROM. Nb : do clearpass() as soon as possible
espwv32::Credentials Storage::decrypt(espwv32::Credentials encryptedCredentials) {
  uint8_t key[32];
  uint8_t iv[16];
  
  esp_aes_context ctx;
  memset( iv, 0, sizeof( iv ) ); //TODO: generate correct Initialisation vector
  memset( key, 0, sizeof( key ) ); //TODO: generate key based on PIN
  esp_aes_init( &ctx );
  esp_aes_setkey( &ctx, key, 256 );

  Credentials credentials;
  int result = esp_aes_crypt_cbc( &ctx, ESP_AES_DECRYPT, sizeof(credentials), iv, (uint8_t*) &encryptedCredentials, (uint8_t*) &credentials );
  esp_aes_free( &ctx );
  //TODO: error handling -> result != 0
  //TODO: clean memory
  return credentials;
}
