#include "Storage.h"
#include "esp_random.h"

using namespace espwv32;

int Storage::_eepromSize = 4096;

// ── PBKDF2-HMAC-SHA256 helper ─────────────────────────────────────────────────

bool Storage::pbkdf2(uint8_t pin[], uint8_t salt[], uint8_t out[]) {
  int rc = mbedtls_pkcs5_pbkdf2_hmac_ext(
    MBEDTLS_MD_SHA256,
    pin, 4,
    salt, SALT_SIZE,
    PBKDF2_ITER,
    HASH_SIZE, out);
  return rc == 0;
}

// ── PIN verifier (salt + PBKDF2 hash) stored in EEPROM ───────────────────────

bool Storage::writeVerifier(uint8_t pin[]) {
  uint8_t salt[SALT_SIZE];
  esp_fill_random(salt, SALT_SIZE);

  uint8_t hash[HASH_SIZE];
  if (!pbkdf2(pin, salt, hash)) return false;

  for (int i = 0; i < SALT_SIZE; i++) EEPROM.write(pinSaltAddr() + i, salt[i]);
  for (int i = 0; i < HASH_SIZE; i++) EEPROM.write(pinHashAddr() + i, hash[i]);
  return true;
}

void Storage::setPinConfigured(uint8_t pin[]) {
  writeVerifier(pin);
  EEPROM.commit();
}

bool Storage::verifyPin(uint8_t pin[]) {
  uint8_t salt[SALT_SIZE], storedHash[HASH_SIZE], computedHash[HASH_SIZE];
  for (int i = 0; i < SALT_SIZE; i++) salt[i]       = EEPROM.read(pinSaltAddr() + i);
  for (int i = 0; i < HASH_SIZE; i++) storedHash[i] = EEPROM.read(pinHashAddr() + i);

  if (!pbkdf2(pin, salt, computedHash)) return false;
  return memcmp(computedHash, storedHash, HASH_SIZE) == 0;
}

bool Storage::resetPin(uint8_t oldPin[], uint8_t newPin[]) {
  if (!verifyPin(oldPin)) return false;

  uint8_t count = getSlotCount();
  for (int i = 0; i < count; i++) {
    Credentials c = read(i, oldPin);
    EEPROM.put(i * sizeof(Credentials), encrypt(c, newPin));
  }
  if (!writeVerifier(newPin)) return false;
  EEPROM.commit();
  return true;
}


bool Storage::deleteSlot(byte index) {
  uint8_t count = getSlotCount();
  if (index >= count) return false;

  // Shift subsequent encrypted slots down by one (no PIN needed — raw copy)
  Credentials tmp;
  for (int i = index; i < count - 1; i++) {
    EEPROM.get((i + 1) * sizeof(Credentials), tmp);
    EEPROM.put(i       * sizeof(Credentials), tmp);
  }
  EEPROM.write(slotCountAddr(), count - 1);
  EEPROM.commit();
  return true;
}

bool Storage::store(byte index, espwv32::Credentials credentials, uint8_t pin[]) {
  EEPROM.put(index * sizeof(credentials), encrypt(credentials, pin));
  // Expand the slot count when a non-empty slot beyond the current count is saved
  if (strlen(credentials.name) > 0 && index >= getSlotCount()) {
    uint8_t newCount = (uint8_t)min((int)index + 1, maxSlots());
    EEPROM.write(slotCountAddr(), newCount);
  }
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
  
  mbedtls_aes_context ctx;
  mbedtls_aes_init( &ctx );
  mbedtls_aes_setkey_enc( &ctx, key, 256 );

  Credentials encryptedCredentials;
  int result = mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_ENCRYPT, sizeof(credentials), iv, (uint8_t*) &credentials, (uint8_t*) &encryptedCredentials );
  mbedtls_aes_free( &ctx );
  //TODO: error handling -> result != 0
  //TODO: clean memory: iv, key, credentials
  return encryptedCredentials;
}

espwv32::Credentials Storage::decrypt(espwv32::Credentials encryptedCredentials, uint8_t pin[]) {
  uint8_t iv[16];
  uint8_t key[32];

  calculateIV(iv, pin);
  calculateKey(key, pin);

  mbedtls_aes_context ctx;
  mbedtls_aes_init( &ctx );
  mbedtls_aes_setkey_dec( &ctx, key, 256 ); //256 bit encryption

  Credentials credentials;
  int result = mbedtls_aes_crypt_cbc( &ctx, MBEDTLS_AES_DECRYPT, sizeof(credentials), iv, (uint8_t*) &encryptedCredentials, (uint8_t*) &credentials );
  mbedtls_aes_free( &ctx );
  //TODO: error handling -> result != 0
  //TODO: clean memory: iv, key, encryptedCredentials
  return credentials;
}


uint8_t* Storage::calculateIV(uint8_t iv[], uint8_t pin[]) {
  memset( iv, 0, 16 );

  const uint8_t pinSize = 4;
  for (int i = 0; i < 16; i++) {
    iv[i] = pin[i % pinSize];
  }
  return iv;
}
uint8_t* Storage::calculateKey(uint8_t key[], uint8_t pin[]) {
  memset( key, 0, 32 );

  const uint8_t pinSize = 4;
  for (int i = 0; i < 32; i++) {
    key[i] = pin[i % pinSize];
  }
  return key;
}
