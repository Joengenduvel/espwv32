#include <EEPROM.h>
#include <mbedtls/aes.h>
#include <mbedtls/pkcs5.h>
#include <Arduino.h>

#ifndef MY_CLASS_Storage
#define MY_CLASS_Storage

namespace espwv32 {

struct Credentials {
  char name[26];
  char username[38];
  char password[32];
};

class Storage {
  public:
    // ── EEPROM layout ─────────────────────────────────────────────────────────
    // [0 .. maxSlots*96-1]      encrypted credential slots
    // [slotCountAddr]            slot count              (1 byte)
    // [pinSaltAddr .. +15]       PBKDF2 random salt      (16 bytes)
    // [pinHashAddr .. +31]       PBKDF2-HMAC-SHA256 hash (32 bytes)
    // [wifiPassAddr .. +31]      WiFi admin password      (32 bytes, plain text)
    //
    // No separate "PIN configured" flag — isPinConfigured() checks whether the
    // salt area is still all 0xFF (erased flash). writeVerifier() always writes
    // a random salt, so any non-0xFF byte means a PIN has been set.
    // ─────────────────────────────────────────────────────────────────────────
    static const int     SALT_SIZE      = 16;
    static const int     HASH_SIZE      = 32;
    static const int     WIFI_PASS_SIZE = 32;  // 31 chars + null terminator
    static const int     PBKDF2_ITER    = 10000;
    static const int     METADATA_SIZE  = 1 + SALT_SIZE + HASH_SIZE + WIFI_PASS_SIZE; // 81 bytes

    static void setSize(int bytes)  { _eepromSize = bytes; _slotCountCacheLoaded = false; }
    static int  maxSlots()          { return (_eepromSize - METADATA_SIZE) / (int)sizeof(Credentials); }
    static int  slotCountAddr()     { return maxSlots() * (int)sizeof(Credentials); }
    static int  pinSaltAddr()       { return slotCountAddr() + 1; }
    static int  pinHashAddr()       { return pinSaltAddr() + SALT_SIZE; }
    static int  wifiPassAddr()      { return pinHashAddr() + HASH_SIZE; }

    Storage();
    ~Storage() {}

    bool        store(byte index, Credentials creds, uint8_t pin[]);
    Credentials read(byte index, uint8_t pin[]);

    // A PIN has been set if the salt area is not all 0xFF (erased flash default).
    // esp_fill_random() making 16 bytes of all 0xFF is a 2^-128 probability.
    bool isPinConfigured() {
      for (int i = 0; i < SALT_SIZE; i++)
        if (EEPROM.read(pinSaltAddr() + i) != 0xFF) return true;
      return false;
    }

    void setPinConfigured(uint8_t pin[]);
    bool verifyPin(uint8_t pin[]);
    bool resetPin(uint8_t oldPin[], uint8_t newPin[]);

    // Deletes slot at index, shifts subsequent slots down, decrements count.
    // Works on raw encrypted bytes — no PIN needed.
    bool deleteSlot(byte index);

    // WiFi admin password — stored in plain text (not sensitive, easily rotatable)
    String getWifiPass() {
      char buf[WIFI_PASS_SIZE];
      for (int i = 0; i < WIFI_PASS_SIZE; i++) buf[i] = (char)EEPROM.read(wifiPassAddr() + i);
      if ((uint8_t)buf[0] == 0xFF) return ""; // uninitialised
      buf[WIFI_PASS_SIZE - 1] = '\0';
      return String(buf);
    }
    void setWifiPass(const String& pass) {
      int len = min((int)pass.length(), WIFI_PASS_SIZE - 1);
      for (int i = 0; i < len; i++) EEPROM.write(wifiPassAddr() + i, (uint8_t)pass[i]);
      EEPROM.write(wifiPassAddr() + len, 0);
      EEPROM.commit();
    }

    uint8_t getSlotCount();

  private:
    static int _eepromSize;
    static uint8_t _slotCountCache;
    static bool _slotCountCacheLoaded;
    static void initSlotCountCache();
    bool writeVerifier(uint8_t pin[]);    // returns false if PBKDF2 fails
    bool pbkdf2(uint8_t pin[], uint8_t salt[], uint8_t out[]); // false on error
    Credentials encrypt(Credentials credentials, uint8_t pin[]);
    Credentials decrypt(Credentials credentials, uint8_t pin[]);
    uint8_t* calculateIV(uint8_t iv[], uint8_t* pin);
    uint8_t* calculateKey(uint8_t key[], uint8_t* pin);
};

}
#endif
