rtc
===

Standalone minimal RTC helper for DS1307/DS3231 over I2C. No RTClib dependency, no dynamic allocation, and no `String` usage. Provides only the essentials: init, running check, compile-time fallback set, explicit set, read, and signed adjustments. Maintained by `pandiyarajk`.

Files
- `library.properties` — Arduino library metadata.
- `src/RTCManager.h/.cpp` — lightweight RTC class.
- `keywords.txt` — Arduino IDE highlighting.
- `examples/Basic/Basic.ino` — minimal usage.
- `LICENSE` — MIT.

API (RTCManager)
- `bool begin()` — init RTC, returns false if not found.
- `bool isRunning()` — true if oscillator runs (checks CH bit).
- `RTCStatus status() const` — last status code.
- `bool setCompileTimeIfStopped()` — set to compile time if stopped.
- `bool setTime(const SimpleDateTime& dt)` — write explicit time.
- `bool adjustMinutes(int16_t delta)` — add/sub minutes (saturates at 2000-01-01).
- `bool adjustSeconds(int32_t delta)` — add/sub seconds (saturates at 2000-01-01).
- `bool readNow(SimpleDateTime& out)` — get current time.

Usage
```cpp
#include <RTCManager.h>

RTCManager rtcMgr;

void setup() {
  Serial.begin(9600);
  if (!rtcMgr.begin()) {
    Serial.println(F("RTC not found"));
    while (1);
  }
  rtcMgr.setCompileTimeIfStopped(); // optional first-time set
}

void loop() {
  SimpleDateTime now;
  if (rtcMgr.readNow(now)) {
    Serial.print(now.hour); Serial.print(':');
    Serial.print(now.minute); Serial.print(':');
    Serial.println(now.second);
  }
  delay(1000);
}
```

Adjust time examples
```cpp
rtcMgr.adjustMinutes(3);    // add 3 minutes
rtcMgr.adjustMinutes(-5);   // subtract 5 minutes
rtcMgr.adjustSeconds(90);   // add 90 seconds
rtcMgr.setTime(SimpleDateTime{2025, 12, 8, 17, 41, 0}); // explicit set
```

Status codes
- `RTC_STATUS_OK`
- `RTC_STATUS_NOT_FOUND`
- `RTC_STATUS_NOT_RUNNING`
- `RTC_STATUS_SET_COMPILE_TIME`

Notes
- Uses `Wire` only; keep coin-cell fresh for persistence.
- DS3231 offers better accuracy than DS1307; both share the same register layout for basic time-keeping.

Installing (local ZIP)
- Zip the `rtc_lib` folder (ensure `library.properties` is at root of zip).
- Arduino IDE: Sketch → Include Library → Add .ZIP Library.

Preparing for Arduino Library Manager
- Set `url` in `library.properties` to the public GitHub repo.
- Commit/push, tag a release matching `version` (e.g., v1.0.0).
- Open an issue at https://github.com/arduino/library-registry with the repo URL.

