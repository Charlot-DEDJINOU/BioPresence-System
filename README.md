# BioPresence System

BioPresence System is a solution combining a FastAPI + SQLite backend and ESP32/Arduino embedded programs to manage employee attendance using double authentication (personal code + fingerprint).

## Double Authentication

The system requires users to enter their personal code on the keypad and then verify their identity using their fingerprint. This two-factor authentication ensures secure and reliable attendance tracking.

## Features

- Employee and fingerprint registration
- Unique code and employee code assignment
- Attendance verification via double authentication and reporting to the API
- Hardware interface: fingerprint sensor, LCD screen, keypad
- Attendance tracking (check-in/check-out) and history

## Project Structure

- `Backend/`: FastAPI API, database, models, and routes
- `System/`: Embedded programs for ESP32/Arduino

## Quick Installation

### Backend

```sh
cd Backend
python -m venv venv
source venv/bin/activate
pip install -r requirements.txt
uvicorn app.main:app --reload
```

### System

- Open `.ino` files in Arduino IDE
- Configure hardware (pins, I2C addresses, WiFi)
- Upload to ESP32 board

## Documentation

- Interactive API: [http://localhost:8000/docs](http://localhost:8000/docs)
- See each folder for more details

## Authors

[Charlot DEDJINOU](https://charlot-dedjinou.vercel.app)