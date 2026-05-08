# espwv32
A password vault you can take with you. Credentials are stored encrypted on the device and sent wirelessly to a connected computer via Bluetooth HID (acts as a keyboard).

## Key Properties
* No client software needed — appears as a Bluetooth keyboard
* 2-factor authentication — BLE pairing PIN + user-defined device PIN
* AES-256 encrypted credential storage
* Secure BLE connection (MITM-protected bonding)
* Web-based account management via on-device Wi-Fi access point

### Current status
* [x] BLE connection requires passcode
* [x] Secure BLE connection
* [x] Send username and password on button push
* [x] Store credentials securely on the device
* [x] Manual managing accounts (via Wi-Fi Admin screen)
* [ ] Proposing new passwords for existing accounts
* [ ] Changing a password manually
* [ ] Fully security checked

---

## Hardware
* Device: https://m5stack.com/products/stick-c
* Arduino API: https://docs.m5stack.com/#/en/arduino/arduino_api

### Device Layout

```
                    ◉ LED   IR 》
                  ┌──────────────┐
                  │ ┌──────────┐ ├─── [B] (right side, large)
                  │ │          │ │
                  │ │  Screen  │ │
                  │ │  80×160  │ │
  [PWR]           │ │          │ │
  (left side) ────┤ └──────────┘ │
  hold 6s=OFF     │ [A]          │    ← front face, bottom-left
                  └──┬───────────┘
                  Grove  USB-C CHG
```

* **Button B** — right-side button; mapped to value change / send
* **Button A** — front face (larger); mapped to navigation / confirm
* **PWR switch** — left side; hold 6 seconds to power off

---

## Screen Flow

### First boot (no PIN configured)
```
┌─────────────┐  BLE pair   ┌────────────┐  BLE auth OK  ┌───────────────┐
│ Start Screen│ ──────────► │ Pin Screen │ ─────────────► │ Set PIN Screen│
└─────────────┘             └────────────┘                └───────────────┘
                                                                  │
                                                          PIN confirmed (A medium ×2)
                                                                  │
                                                                  ▼
                                                         ┌──────────────────┐
                                                         │  WiFi Admin      │  ← add accounts here
                                                         └──────────────────┘
                                                                  │
                                                          A (any press) or client disconnects
                                                                  │
                                                                  ▼
                                                         ┌──────────────────┐
                                                         │ Account Selection│
                                                         └──────────────────┘
```

### Normal use (PIN configured, accounts exist)
```
┌─────────────┐  BLE pair   ┌────────────┐   BLE auth OK  ┌────────────┐
│ Start Screen│ ──────────► │ Pin Screen │ ─────────────► │ Lock Screen│
└─────────────┘             └────────────┘                └────────────┘
                                                                 │
                                                         PIN entered (A medium)
                                                                 │
                                                                 ▼
                                                        ┌──────────────────┐
                                                        │ Account Selection│
                                                        └──────────────────┘
                                                                 │
                                                          A long press
                                                                 │
                                                                 ▼
                                                        ┌──────────────────┐
                                                        │  WiFi Admin      │
                                                        └──────────────────┘
                                                                 │
                                                         A (any press) or client disconnects
                                                                 │
                                                                 ▼
                                                        ┌──────────────────┐
                                                        │ Account Selection│
                                                        └──────────────────┘
```

### PIN configured but no accounts yet
```
Lock Screen ──(PIN entered)──► WiFi Admin   (same as first-boot WiFi Admin step)
```

> If BLE disconnects at any point, the device returns to the **Start Screen** (except during WiFi Admin, where BLE is intentionally disconnected).

---

## Screens

### 1. Start Screen
Shown on boot and after a BLE disconnect. Displays the device logo and device ID.

![Start screen](IMG_3459.JPG)

```
┌────────────────────────┐
│ 🔋 87%        ○        │  ← battery & BLE status
│                        │
│      ESPWV32           │
│   Password Vault       │
│                        │
│  ID: 3a4f9bc1          │  ← unique device ID
│                        │
│  Waiting for BLE...    │
└────────────────────────┘
```

**Waiting state** — no button interaction. Device is advertising over BLE.

---

### 2. Pin Screen (BLE Pairing)
Shown automatically when a new Bluetooth host tries to pair. Displays the numeric passkey that must be confirmed on the connecting device.

![Pin entry screen](IMG_3461.JPG)

```
┌────────────────────────┐
│ 🔋 87%        ●        │  ← BLE connecting
│                        │
│   Confirm PIN on       │
│   your device:         │
│                        │
│      3 2 5 1           │  ← passkey shown by BLE stack
│                        │
│  Confirm on host...    │
└────────────────────────┘
```

**Waiting state** — no button interaction needed. After the host confirms the PIN, pairing completes and the device advances to the Lock Screen.

---

### 3. Lock Screen
The user must enter their personal 4-digit PIN before credentials can be accessed. This PIN is also the encryption key for stored credentials.

![Lock screen](IMG_3463.JPG)

```
┌────────────────────────┐
│ 🔋 87%        ●        │
│                        │
│   Enter PIN:           │
│                        │
│    [ 1 ][ 2 ][ 3 ][ 4 ]│  ← active digit highlighted in red
│                        │
│  [A]cursor  [B]+  [A~] │
└────────────────────────┘
```

| Button | Action |
|--------|--------|
| **A press** | Move cursor right (cycle through 4 digits) |
| **B press** | Increment selected digit (0 → 9 → 0) |
| **A medium press** (500 ms) | Confirm PIN → advance to Account Selection (or WiFi Admin if no accounts) |

---

### 3a. Set PIN Screen *(first boot only)*
Shown the very first time the device is paired, before any PIN has been configured. The user sets a new 4-digit PIN and must confirm it by entering it a second time.

```
┌────────────────────────┐       ┌────────────────────────┐
│ 🔋 87%        ●        │       │ 🔋 87%        ●        │
│                        │  ok   │                        │
│  Set new PIN:          │ ────► │  Confirm new PIN:      │
│                        │       │                        │
│    [ 0 ][ 0 ][ 0 ][ 0 ]│       │    [ 0 ][ 0 ][ 0 ][ 0 ]│
│                        │       │                        │
│  [A]cursor  [B]+  [A~] │       │  [A]cursor  [B]+  [A~] │
└────────────────────────┘       └────────────────────────┘
```

| Button | Action |
|--------|--------|
| **A press** | Move cursor right |
| **B press** | Increment selected digit |
| **A medium press** (500 ms) | Confirm phase / confirm PIN |

If the two entries do not match, a "PIN mismatch" error is shown for 1.5 s and the screen resets to phase 1. On success the PIN-configured flag is saved and the device proceeds to **WiFi Admin** to add accounts.

---

### 4. Account Selection Screen
Browse and send stored credentials.

![Account Selection](IMG_3469.JPG)

```
┌────────────────────────┐
│ 🔋 87%   ●    [U→P]   │  ← send mode indicator
│                        │
│  2 / 10                │  ← slot index
│  ──────────────────    │
│  Account 2             │  ← account name
│  User 2                │  ← username
│  ••••••••              │  ← password (masked)
│                        │
│[A]next [B]send         │
└────────────────────────┘
```

| Button | Action |
|--------|--------|
| **A press** | Next account |
| **A medium press** (500 ms) | Previous account |
| **B press** | Send credentials via BLE (types as keyboard) |
| **B medium press** (500 ms) | Cycle send mode: `U→P` (username + TAB + password) → `U` (username only) → `P` (password only) |
| **B long press** (1000 ms) | Open **WiFi Admin Screen** |

The active send mode is highlighted in the top-right corner of the screen.

---

### 5. WiFi Admin Screen *(new)*
Starts a Wi-Fi Access Point on the device. Any device that connects is automatically redirected to a web page where all 10 credential slots can be viewed and edited.

```
┌────────────────────────┐
│ WiFi Admin             │
│                        │
│ SSID: ESPWV32-Admin    │
│ Pass: vault1234        │
│                        │
│ IP:   192.168.4.1      │
│                        │
│ [A] Back               │
└────────────────────────┘
```

**Web interface** (rendered in browser at `http://192.168.4.1`):

```
╔══════════════════════════════════╗
║  🔒 ESPWV32 Account Manager      ║
╠══════════════════════════════════╣
║ ┌──────────────────────────────┐ ║
║ │ Slot 0                       │ ║
║ │ Name     [Account 1        ] │ ║
║ │ Username [User 1           ] │ ║
║ │ Password [••••••••••       ] │ ║
║ │                   [  Save  ] │ ║
║ └──────────────────────────────┘ ║
║ ┌──────────────────────────────┐ ║
║ │ Slot 1   ...                 │ ║
║ └──────────────────────────────┘ ║
╚══════════════════════════════════╝
```

| Button | Action |
|--------|--------|
| **A (any press/hold)** | Stop AP and return to Account Selection |

**Web interface** (`http://192.168.4.1`):
* Lists all 10 account slots with Name, Username and Password fields
* Each slot has its own **Save** button
* Credentials are encrypted with your device PIN before being written to storage
* A captive portal DNS server redirects all hostnames to the admin page — most phones will open it automatically when connecting to the AP

---

## Button Quick Reference

| Screen | A press | A medium | A long | B press | B medium | B long |
|--------|---------|----------|--------|---------|----------|--------|
| Start | — | — | — | — | — | — |
| Pin | — | — | — | — | — | — |
| Set PIN | Cursor → | Confirm phase | — | Digit ++ | — | — |
| Lock | Cursor → | Confirm PIN | — | Digit ++ | — | — |
| Accounts | Next | Prev | — | Send | Cycle mode | WiFi Admin |
| WiFi Admin | Back | Back | Back | — | — | — |

---

## Storage & Encryption

Credentials are stored in the ESP32's EEPROM (flash). Each slot holds:

| Field | Max length |
|-------|-----------|
| Name | 25 chars |
| Username | 37 chars |
| Password | 31 chars |

Each slot (96 bytes) is encrypted with **AES-256-CBC** using a key and IV derived from the user's 4-digit PIN.

### EEPROM layout

```
[ slot 0 ][ slot 1 ] … [ slot N-1 ][ PIN flag ][ slot count ]
  96 B       96 B            96 B      1 B           1 B
```

**`MAX_SLOTS`** is derived automatically from the EEPROM size configured in `setup()`:

```
MAX_SLOTS = (EEPROM_SIZE - 2) / 96
```

With the default `EEPROM.begin(4096)` this gives **42 slots**. To add more, increase the value in both `setup()` and `Storage::EEPROM_SIZE` — `MAX_SLOTS` and all address constants update automatically.

The WiFi Admin page always shows all saved slots **plus one empty slot** for adding a new account, up to `MAX_SLOTS`. Saving a non-empty slot at index ≥ current count expands the count automatically.

Changing the PIN without re-encrypting the data will make all stored credentials unreadable.

---

## High Level Architecture
![Architecture](architecture.png)

## Sequences
![Sequences](sequence.png)

---

## Quality
[![CodeFactor](https://www.codefactor.io/repository/github/joengenduvel/espwv32/badge)](https://www.codefactor.io/repository/github/joengenduvel/espwv32)

---

## Inspiring reads
* https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/mbedtls/mbedtls/aes.h
* https://hackaday.com/2020/02/19/password-keeper-uses-off-the-shelf-formfactor/
