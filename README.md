# espwv32
A password vault you can take with you.

## Key Properties
* No client software needed
* 2 factor authentication
* Encrypted storage
* Secure communication

### Current status
* [x] BLE connection requires passcode
* [x] Secure BLE connection
* [x] Send username and password on button push
* [x] Store credentials securely on the device
* [ ] Manual managing accounts
* [ ] Proposing new passwords for existing accounts
* [ ] Changing a password manually
* [ ] Fully security checked


## Hardware
https://m5stack.com/products/stick-c
https://docs.m5stack.com/#/en/arduino/arduino_api

## Screens
### Start Screen
![alt text](IMG_3459.JPG "Start screen")
### Bluetooth Connection PIN
![alt text](IMG_3461.JPG "Pin entry screen")
### Locked, waiting for User PIN
![alt text](IMG_3463.JPG "Lock screen")
### Selecting an Account
![alt text](IMG_3469.JPG "Account Selection")

## High Level Acrchitecture
![alt text](architecture.png "Logo Title Text 1")

## Sequences
![alt text](sequence.png "Logo Title Text 1")

## Quality
[![CodeFactor](https://www.codefactor.io/repository/github/joengenduvel/espwv32/badge)](https://www.codefactor.io/repository/github/joengenduvel/espwv32)

## Inspiring reads:
* https://github.com/espressif/arduino-esp32/blob/master/tools/sdk/include/mbedtls/mbedtls/aes.h
* https://hackaday.com/2020/02/19/password-keeper-uses-off-the-shelf-formfactor/
