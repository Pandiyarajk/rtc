#include "../rtc.h"

// DS1307/DS3231 share the same base address
static const uint8_t RTC_I2C_ADDR = 0x68;

static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }

RTCManager::RTCManager() : _i2cAddr(RTC_I2C_ADDR), _initialized(false), _status(RTC_STATUS_NOT_FOUND) {}

bool RTCManager::begin() {
  Wire.begin();
  // Probe address
  Wire.beginTransmission(_i2cAddr);
  if (Wire.endTransmission() != 0) {
    _status = RTC_STATUS_NOT_FOUND;
    _initialized = false;
    return false;
  }
  _initialized = true;
  _status = RTC_STATUS_OK;
  return true;
}

bool RTCManager::isRunning() {
  if (!_initialized) return false;
  uint8_t sec;
  if (!readRegisters(0x00, &sec, 1)) return false;
  bool running = !(sec & 0x80); // CH bit
  _status = running ? RTC_STATUS_OK : RTC_STATUS_NOT_RUNNING;
  return running;
}

// --- Compile-time parsing helpers (no String use) ---
static uint8_t parseMonth(const char* m) {
  if (m[0]=='J' && m[1]=='a') return 1;
  if (m[0]=='F') return 2;
  if (m[0]=='M' && m[2]=='r') return 3;
  if (m[0]=='A' && m[1]=='p') return 4;
  if (m[0]=='M' && m[2]=='y') return 5;
  if (m[0]=='J' && m[2]=='n') return 6;
  if (m[0]=='J' && m[2]=='l') return 7;
  if (m[0]=='A' && m[1]=='u') return 8;
  if (m[0]=='S') return 9;
  if (m[0]=='O') return 10;
  if (m[0]=='N') return 11;
  return 12;
}

static void parseDateTimeConstexpr(SimpleDateTime& dt) {
  // __DATE__ format: "Mmm dd yyyy"
  // __TIME__ format: "hh:mm:ss"
  const char* d = __DATE__;
  const char* t = __TIME__;
  dt.month = parseMonth(d);
  dt.day   = (uint8_t)((d[4]==' ' ? d[5]-'0' : (d[4]-'0')*10 + (d[5]-'0')));
  dt.year  = (uint16_t)((d[7]-'0')*1000 + (d[8]-'0')*100 + (d[9]-'0')*10 + (d[10]-'0'));
  dt.hour   = (uint8_t)((t[0]-'0')*10 + (t[1]-'0'));
  dt.minute = (uint8_t)((t[3]-'0')*10 + (t[4]-'0'));
  dt.second = (uint8_t)((t[6]-'0')*10 + (t[7]-'0'));
}

bool RTCManager::setCompileTimeIfStopped() {
  if (!_initialized) return false;
  if (!isRunning()) {
    SimpleDateTime dt;
    parseDateTimeConstexpr(dt);
    setTime(dt);
    _status = RTC_STATUS_SET_COMPILE_TIME;
  } else {
    _status = RTC_STATUS_OK;
  }
  return true;
}

bool RTCManager::setTime(const SimpleDateTime& dt) {
  if (!_initialized) return false;
  if (!writeRaw(dt)) return false;
  _status = RTC_STATUS_OK;
  return true;
}

bool RTCManager::adjustMinutes(int16_t deltaMinutes) {
  return adjustSeconds((int32_t)deltaMinutes * 60);
}

bool RTCManager::adjustSeconds(int32_t deltaSeconds) {
  if (!_initialized) return false;
  SimpleDateTime now;
  if (!readRaw(now)) return false;
  uint32_t ts = toUnix(now);
  const uint32_t base = 946684800UL; // 2000-01-01
  if (deltaSeconds < 0) {
    uint32_t d = (uint32_t)(-deltaSeconds);
    ts = (d > ts) ? base : ts - d;
  } else {
    ts += (uint32_t)deltaSeconds;
  }
  SimpleDateTime updated = fromUnix(ts);
  if (!writeRaw(updated)) return false;
  _status = RTC_STATUS_OK;
  return true;
}

bool RTCManager::readNow(SimpleDateTime& out) {
  if (!_initialized) return false;
  return readRaw(out);
}

// --- I2C helpers ---
bool RTCManager::writeRegisters(uint8_t start, const uint8_t* data, uint8_t len) {
  Wire.beginTransmission(_i2cAddr);
  Wire.write(start);
  Wire.write(data, len);
  return Wire.endTransmission() == 0;
}

bool RTCManager::readRegisters(uint8_t start, uint8_t* data, uint8_t len) {
  Wire.beginTransmission(_i2cAddr);
  Wire.write(start);
  if (Wire.endTransmission() != 0) return false;
  uint8_t read = Wire.requestFrom(_i2cAddr, len);
  if (read != len) return false;
  for (uint8_t i=0; i<len; ++i) data[i] = Wire.read();
  return true;
}

bool RTCManager::readRaw(SimpleDateTime& out) {
  uint8_t buf[7];
  if (!readRegisters(0x00, buf, 7)) return false;
  out.second = bcd2bin(buf[0] & 0x7F);
  out.minute = bcd2bin(buf[1]);
  out.hour   = bcd2bin(buf[2] & 0x3F); // 24h
  out.day    = bcd2bin(buf[4]);
  out.month  = bcd2bin(buf[5] & 0x1F);
  out.year   = 2000 + bcd2bin(buf[6]);
  return true;
}

bool RTCManager::writeRaw(const SimpleDateTime& dt) {
  uint8_t buf[7];
  buf[0] = bin2bcd(dt.second & 0x7F); // ensure CH=0
  buf[1] = bin2bcd(dt.minute);
  buf[2] = bin2bcd(dt.hour);          // 24h
  buf[3] = 1;                         // day of week (not used)
  buf[4] = bin2bcd(dt.day);
  buf[5] = bin2bcd(dt.month);
  buf[6] = bin2bcd((uint8_t)(dt.year >= 2000 ? dt.year - 2000 : 0));
  return writeRegisters(0x00, buf, 7);
}

// --- Time conversion helpers ---
static const uint8_t daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};

static bool isLeap(uint16_t y) {
  return ((y % 4 == 0) && (y % 100 != 0)) || (y % 400 == 0);
}

uint32_t RTCManager::toUnix(const SimpleDateTime& dt) const {
  // Based on DateTime logic; supports 2000-2099
  uint16_t days = 0;
  for (uint16_t y = 2000; y < dt.year; ++y) {
    days += isLeap(y) ? 366 : 365;
  }
  for (uint8_t m = 1; m < dt.month; ++m) {
    days += daysInMonth[m - 1];
    if (m == 2 && isLeap(dt.year)) days += 1;
  }
  days += (dt.day - 1);
  uint32_t seconds = (uint32_t)days * 86400UL;
  seconds += (uint32_t)dt.hour * 3600UL;
  seconds += (uint32_t)dt.minute * 60UL;
  seconds += dt.second;
  return seconds + 946684800UL; // seconds from 1970 to 2000-01-01
}

SimpleDateTime RTCManager::fromUnix(uint32_t ts) const {
  SimpleDateTime dt;
  ts -= 946684800UL; // back to 2000-01-01
  uint16_t days = ts / 86400UL;
  uint32_t rem = ts % 86400UL;
  dt.hour = rem / 3600;
  rem %= 3600;
  dt.minute = rem / 60;
  dt.second = rem % 60;

  uint16_t y = 2000;
  while (true) {
    uint16_t daysInYear = isLeap(y) ? 366 : 365;
    if (days < daysInYear) break;
    days -= daysInYear;
    ++y;
  }
  dt.year = y;
  uint8_t m = 1;
  while (true) {
    uint8_t dim = daysInMonth[m - 1];
    if (m == 2 && isLeap(y)) dim++;
    if (days < dim) break;
    days -= dim;
    ++m;
  }
  dt.month = m;
  dt.day = days + 1;
  return dt;
}

