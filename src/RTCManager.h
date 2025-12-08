#pragma once
#include <Arduino.h>
#include <Wire.h>

// Lightweight date-time container (year 2000-2099)
struct SimpleDateTime {
  uint16_t year;   // 2000-2099
  uint8_t  month;  // 1-12
  uint8_t  day;    // 1-31
  uint8_t  hour;   // 0-23
  uint8_t  minute; // 0-59
  uint8_t  second; // 0-59
};

enum RTCStatus : uint8_t {
  RTC_STATUS_OK = 0,
  RTC_STATUS_NOT_FOUND,
  RTC_STATUS_NOT_RUNNING,
  RTC_STATUS_SET_COMPILE_TIME
};

class RTCManager {
public:
  RTCManager();

  // Initialize the RTC (DS1307/DS3231). Returns true if the RTC responds on I2C.
  bool begin();

  // Returns true if the RTC oscillator is running.
  bool isRunning();

  // Returns last recorded status.
  RTCStatus status() const { return _status; }

  // If the RTC is stopped, set it to compile time. Returns true on success.
  bool setCompileTimeIfStopped();

  // Set an explicit time.
  bool setTime(const SimpleDateTime& dt);

  // Adjust by N minutes (positive or negative).
  bool adjustMinutes(int16_t deltaMinutes);

  // Adjust by N seconds (positive or negative).
  bool adjustSeconds(int32_t deltaSeconds);

  // Read current time into out parameter; returns false if unavailable.
  bool readNow(SimpleDateTime& out);

private:
  bool writeRegisters(uint8_t start, const uint8_t* data, uint8_t len);
  bool readRegisters(uint8_t start, uint8_t* data, uint8_t len);
  bool readRaw(SimpleDateTime& out);
  bool writeRaw(const SimpleDateTime& dt);
  uint32_t toUnix(const SimpleDateTime& dt) const;
  SimpleDateTime fromUnix(uint32_t ts) const;

  uint8_t _i2cAddr;
  bool _initialized;
  RTCStatus _status;
};

