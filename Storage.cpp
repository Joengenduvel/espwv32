#include "Storage.h"

using namespace espwv32;

bool Storage::store(byte index, espwv32::Credentials credentials, uint8_t pin[]) {
  //todo: validate index

  EEPROM.put(index * sizeof(credentials), encrypt(credentials, pin));
  return true;
}
espwv32::Credentials Storage::read(byte index, uint8_t pin[]) {
  //todo: validate index
  
  Serial.printf("Reading index, %d with pin %d%d%d%d\n", index, pin[0], pin[1], pin[2], pin[3]);
  Credentials credentials;
  return decrypt(EEPROM.get(index * sizeof(credentials), credentials), pin);
}

espwv32::Credentials Storage::encrypt(espwv32::Credentials credentials, uint8_t pin[]) {
  uint8_t iv[16];
  uint8_t key[32];

  calculateIV(iv, pin);
  calculateKey(key, pin);
  
  esp_aes_context ctx;
  esp_aes_init( &ctx );
  esp_aes_setkey( &ctx, key, 256 );

  Credentials encryptedCredentials;
  int result = esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, sizeof(credentials), iv, (uint8_t*) &credentials, (uint8_t*) &encryptedCredentials );
  esp_aes_free( &ctx );
  //TODO: error handling -> result != 0
  //TODO: clean memory: iv, key, credentials
  return encryptedCredentials;
}

espwv32::Credentials Storage::decrypt(espwv32::Credentials encryptedCredentials, uint8_t pin[]) {
  uint8_t iv[16];
  uint8_t key[32];

  calculateIV(iv, pin);
  calculateKey(key, pin);

  esp_aes_context ctx;
  esp_aes_init( &ctx );
  esp_aes_setkey( &ctx, key, 256 ); //256 bit encryption

  Credentials credentials;
  int result = esp_aes_crypt_cbc( &ctx, ESP_AES_DECRYPT, sizeof(credentials), iv, (uint8_t*) &encryptedCredentials, (uint8_t*) &credentials );
  esp_aes_free( &ctx );
  //TODO: error handling -> result != 0
  //TODO: clean memory: iv, key, encryptedCredentials
  return credentials;
}


uint8_t* Storage::calculateIV(uint8_t iv[], uint8_t pin[]) {
  memset( iv, 0, sizeof( iv ) );

  char pinSize = sizeof(pin) / sizeof(uint8_t);
  for (int i = 0; i < 16; i++) {
    iv[i] = pin[i % pinSize]; //TODO: find better expansion algorithm
  }
  return iv;
}
uint8_t* Storage::calculateKey(uint8_t key[], uint8_t pin[]) {
  memset( key, 0, sizeof( key ) );

  char pinSize = sizeof(pin) / sizeof(uint8_t);
  for (int i = 0; i < 32; i++) {
    key[i] = pin[i % pinSize]; //TODO: find better expansion algorithm
  }
  return key;
}
