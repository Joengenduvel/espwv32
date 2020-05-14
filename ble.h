#include <BLEDevice.h>
#include <BLEHIDDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLESecurity.h>
#include <Print.h>

#ifndef MY_CLASS_BLEKeyboard // include guard
#define MY_CLASS_BLEKeyboard

namespace ble {

//pnp https://www.bluetooth.com/xml-viewer/?src=https://www.bluetooth.com/wp-content/uploads/Sitecore-Media-Library/Gatt/Xml/Characteristics/org.bluetooth.characteristic.pnp_id.xml
#define SIG 0x02 //vendor ID source = USB Implementer’s Forum assigned Vendor ID value
#define VENDOR_ID 0xe502
#define PRODUCT_ID 0xa111
#define PRODUCT_VERSION 0x0210

//TODO: make private
static const uint8_t _hidReportDescriptor[] = {
  USAGE_PAGE(1),      0x01,          // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,          // USAGE (Keyboard)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  // ------------------------------------------------- Keyboard
  REPORT_ID(1),       0x01,          //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,          //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,          //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,          //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,          //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  REPORT_COUNT(1),    0x05,          //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,          //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,          //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,          //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,          //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,          //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,          //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,          //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x06,          //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,          //   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(1), 0x65,          //   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),      0x07,          //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,          //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x65,          //   USAGE_MAXIMUM (0x65)
  HIDINPUT(1),        0x00,          //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0),                 // END_COLLECTION
  // ------------------------------------------------- Media Keys
  USAGE_PAGE(1),      0x0C,          // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,          // USAGE (Consumer Control)
  COLLECTION(1),      0x01,          // COLLECTION (Application)
  REPORT_ID(1),       0x02,          //   REPORT_ID (3)
  USAGE_PAGE(1),      0x0C,          //   USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1), 0x00,          //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,          //   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,          //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x10,          //   REPORT_COUNT (16)
  USAGE(1),           0xB5,          //   USAGE (Scan Next Track)     ; bit 0: 1
  USAGE(1),           0xB6,          //   USAGE (Scan Previous Track) ; bit 1: 2
  USAGE(1),           0xB7,          //   USAGE (Stop)                ; bit 2: 4
  USAGE(1),           0xCD,          //   USAGE (Play/Pause)          ; bit 3: 8
  USAGE(1),           0xE2,          //   USAGE (Mute)                ; bit 4: 16
  USAGE(1),           0xE9,          //   USAGE (Volume Increment)    ; bit 5: 32
  USAGE(1),           0xEA,          //   USAGE (Volume Decrement)    ; bit 6: 64
  USAGE(2),           0x23, 0x02,    //   Usage (WWW Home)            ; bit 7: 128
  USAGE(2),           0x94, 0x01,    //   Usage (My Computer) ; bit 0: 1
  USAGE(2),           0x92, 0x01,    //   Usage (Calculator)  ; bit 1: 2
  USAGE(2),           0x2A, 0x02,    //   Usage (WWW fav)     ; bit 2: 4
  USAGE(2),           0x21, 0x02,    //   Usage (WWW search)  ; bit 3: 8
  USAGE(2),           0x26, 0x02,    //   Usage (WWW stop)    ; bit 4: 16
  USAGE(2),           0x24, 0x02,    //   Usage (WWW back)    ; bit 5: 32
  USAGE(2),           0x83, 0x01,    //   Usage (Media sel)   ; bit 6: 64
  USAGE(2),           0x8A, 0x01,    //   Usage (Mail)        ; bit 7: 128
  HIDINPUT(1),        0x02,          //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0)                  // END_COLLECTION
};

//TODO: make configurable
//https://cdn.sparkfun.com/datasheets/Wireless/Bluetooth/RN-HID-User-Guide-v1.0r.pdf
#define SHIFT 0x80
const uint8_t _asciimap[128] PROGMEM =
{
  0x00,             // NUL
  0x00,             // SOH
  0x00,             // STX
  0x00,             // ETX
  0x00,             // EOT
  0x00,             // ENQ
  0x00,             // ACK
  0x00,             // BEL
  0x2a,     // BS Backspace
  0x2b,     // TAB  Tab
  0x28,     // LF Enter
  0x00,             // VT
  0x00,             // FF
  0x00,             // CR
  0x00,             // SO
  0x00,             // SI
  0x00,             // DEL
  0x00,             // DC1
  0x00,             // DC2
  0x00,             // DC3
  0x00,             // DC4
  0x00,             // NAK
  0x00,             // SYN
  0x00,             // ETB
  0x00,             // CAN
  0x00,             // EM
  0x00,             // SUB
  0x00,             // ESC
  0x00,             // FS
  0x00,             // GS
  0x00,             // RS
  0x00,             // US

  0x2c,      //  ' '
  0x1e | SHIFT,  // !
  0x34 | SHIFT,  // "
  0x20 | SHIFT,  // #
  0x21 | SHIFT,  // $
  0x22 | SHIFT,  // %
  0x24 | SHIFT,  // &
  0x34,          // '
  0x26 | SHIFT,  // (
  0x27 | SHIFT,  // )
  0x25 | SHIFT,  // *
  0x2e | SHIFT,  // +
  0x36,          // ,
  0x2d,          // -
  0x37,          // .
  0x38,          // /
  0x27,          // 0
  0x1e,          // 1
  0x1f,          // 2
  0x20,          // 3
  0x21,          // 4
  0x22,          // 5
  0x23,          // 6
  0x24,          // 7
  0x25,          // 8
  0x26,          // 9
  0x33 | SHIFT,    // :
  0x33,          // ;
  0x36 | SHIFT,    // <
  0x2e,          // =
  0x37 | SHIFT,    // >
  0x38 | SHIFT,    // ?
  0x1f | SHIFT,    // @
  0x04 | SHIFT,    // A
  0x05 | SHIFT,    // B
  0x06 | SHIFT,    // C
  0x07 | SHIFT,    // D
  0x08 | SHIFT,    // E
  0x09 | SHIFT,    // F
  0x0a | SHIFT,    // G
  0x0b | SHIFT,    // H
  0x0c | SHIFT,    // I
  0x0d | SHIFT,    // J
  0x0e | SHIFT,    // K
  0x0f | SHIFT,    // L
  0x10 | SHIFT,    // M
  0x11 | SHIFT,    // N
  0x12 | SHIFT,    // O
  0x13 | SHIFT,    // P
  0x14 | SHIFT,    // Q
  0x15 | SHIFT,    // R
  0x16 | SHIFT,    // S
  0x17 | SHIFT,    // T
  0x18 | SHIFT,    // U
  0x19 | SHIFT,    // V
  0x1a | SHIFT,    // W
  0x1b | SHIFT,    // X
  0x1c | SHIFT,    // Y
  0x1d | SHIFT,    // Z
  0x2f,          // [
  0x31,          // bslash
  0x30,          // ]
  0x23 | SHIFT,  // ^
  0x2d | SHIFT,  // _
  0x35,          // `
  0x04,          // a
  0x05,          // b
  0x06,          // c
  0x07,          // d
  0x08,          // e
  0x09,          // f
  0x0a,          // g
  0x0b,          // h
  0x0c,          // i
  0x0d,          // j
  0x0e,          // k
  0x0f,          // l
  0x10,          // m
  0x11,          // n
  0x12,          // o
  0x13,          // p
  0x14,          // q
  0x15,          // r
  0x16,          // s
  0x17,          // t
  0x18,          // u
  0x19,          // v
  0x1a,          // w
  0x1b,          // x
  0x1c,          // y
  0x1d,          // z
  0x2f | SHIFT,  // {
  0x31 | SHIFT,  // |
  0x30 | SHIFT,  // }
  0x35 | SHIFT,  // ~
  0       // DEL
};

class BLEKeyboardCallbacks {
  public:
    virtual void authenticationInfo(uint32_t pin = 0);
    virtual void connected();
    virtual void disconnected();
};

class BLEKeyboard : private BLEServerCallbacks,  private BLESecurityCallbacks, public Print {
  private:
    typedef struct
    {
      uint8_t modifiers;
      uint8_t reserved;
      uint8_t keys[6];
    } KeyReport;

    std::string deviceName;
    std::string deviceManufacturer;
    BLEHIDDevice* hid;
    BLEServer* pServer;
    BLEKeyboardCallbacks* callbacks;
    bool connected = false;
    BLECharacteristic* inputKeyboard;
    KeyReport _keyReport;

  public:
    BLEKeyboard(std::string deviceName = "ESPWV32", std::string deviceManufacturer = "Espressif");

    ~BLEKeyboard();

    void onConnect(BLEServer* pServer);

    void onDisconnect(BLEServer* pServer);

    uint32_t onPassKeyRequest();

    void onPassKeyNotify(uint32_t pass_key);

    bool onSecurityRequest();

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl);

    bool onConfirmPIN(unsigned int v);

    void disconnect() ;

    bool isConnected();

    char *bda2str(const uint8_t* bda, char *str, size_t size);

    void sendReport(KeyReport* keys);

    size_t press(uint8_t k);

    size_t release(uint8_t k);
    void releaseAll(void);


    size_t write(uint8_t c);

    size_t write(const uint8_t *buffer, size_t size) ;

    void setBatteryLevel(uint8_t level) ;

    void setCallbacks(BLEKeyboardCallbacks* callbacks);

};
}
#endif /* MY_CLASS_BLEKeyboard */
