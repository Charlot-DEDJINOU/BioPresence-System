# System

This folder contains embedded programs for microcontrollers (ESP32/Arduino) that interface with the fingerprint sensor, keypad, LCD screen, and communicate with the Backend API.

## Double Authentication

The main program implements double authentication: users must enter their personal code on the keypad and then verify their identity using their fingerprint. This ensures secure attendance registration and prevents unauthorized access.

## Subfolders

- `Main/`: Main program for attendance registration and verification via double authentication (code + fingerprint) and API.
- `StoreAndFingerVerification/`: Local fingerprint registration and verification.
- `ScannerI2C/`: I2C address scanner for detecting connected devices.
- `VerifyInternetConnection/`: WiFi internet connection check.
- `GetFingerPrintInformations/`: Reading fingerprint sensor information.

## Dependencies

- Arduino libraries: `Wire`, `LiquidCrystal_I2C`, `Adafruit_Fingerprint`, `Keypad_I2C`, `WiFi`, `HTTPClient`, `ArduinoJson`
- Hardware: ESP32, fingerprint sensor, I2C LCD screen, I2C keypad

## Usage

1. Open the relevant `.ino` file in the Arduino IDE.
2. Check pin assignments and I2C addresses according to your setup.
3. Upload to the ESP32 board.
4. Follow instructions displayed on the LCD screen.

---
