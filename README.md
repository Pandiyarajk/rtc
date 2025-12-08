RTC Basics (Arduino/RTClib)
===========================

What an RTC does
- Keeps calendar time with battery backup when the MCU sleeps or loses power.
- Common chips: DS1307 (I2C, minute-level accuracy), DS3231 (I2C, temperature-compensated, much more accurate), PCF8523 (low-power).

Wiring (I2C)
- VCC to 5V (or 3.3V if your board/chip supports it), GND to ground.
- SDA → Arduino SDA pin (A4 on classic Uno), SCL → Arduino SCL pin (A5 on classic Uno).
- Add the coin cell/battery so time persists when main power is off.

RTClib quick reference
- Create an instance: `RTC_DS1307 rtc;` (or `RTC_DS3231`, `RTC_PCF8523`).
- `rtc.begin()` → returns `false` if the chip isn’t reachable on I2C.
- `rtc.isrunning()` → `false` if oscillator stopped or time is unset.
- `rtc.now()` → returns `DateTime` with current time.
- `rtc.adjust(DateTime dt)` → writes a new time to the RTC (persists with battery).
- `DateTime(F(__DATE__), F(__TIME__))` → compile-time timestamp.
- `TimeSpan(days, hours, minutes, seconds)` → add/subtract durations (`DateTime + TimeSpan`).

Minimal setup pattern
```cpp
#include <RTClib.h>
RTC_DS1307 rtc;

void setup() {
  Serial.begin(9600);
  if (!rtc.begin()) {
    Serial.println(F("Couldn't find RTC"));
    while (1);
  }

  if (!rtc.isrunning()) {
    Serial.println(F("RTC not running, setting to compile time"));
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}
```

Read the time
```cpp
DateTime now = rtc.now();
Serial.print(now.year());  Serial.print('/');
Serial.print(now.month()); Serial.print('/');
Serial.print(now.day());   Serial.print(' ');
Serial.print(now.hour());  Serial.print(':');
Serial.print(now.minute());Serial.print(':');
Serial.println(now.second());
```

Set to an explicit timestamp
```cpp
rtc.adjust(DateTime(2025, 12, 8, 17, 41, 0)); // YYYY, M, D, H, M, S
```

Add or subtract time (e.g., adjust by minutes)
```cpp
DateTime current = rtc.now();
rtc.adjust(current + TimeSpan(0, 0, 5, 0));   // add 5 minutes
rtc.adjust(current + TimeSpan(0, 0, -3, 0));  // subtract 3 minutes
```

Check for missing RTC or stopped oscillator
```cpp
if (!rtc.begin()) {
  Serial.println(F("RTC missing"));
}
if (!rtc.isrunning()) {
  Serial.println(F("Oscillator stopped; set the time"));
}
```

Epoch and convenience helpers
```cpp
DateTime now = rtc.now();
uint32_t ts = now.unixtime(); // seconds since 1970-01-01
DateTime later(ts + 3600);    // 1 hour later via unix time
```

Accuracy and drift
- DS1307 can drift (tens of seconds per day). DS3231 is far better (±2 ppm typical).
- For DS1307, plan to resync periodically (manual command, NTP gateway, or GPS).
- Ensure the coin cell is good; dead battery means time will reset.

Daylight-saving or offsets
- Store local offset separately; keep RTC in UTC if you can, then apply offsets in code.
- For fixed offsets, you can adjust with `TimeSpan` when displaying or on demand.

