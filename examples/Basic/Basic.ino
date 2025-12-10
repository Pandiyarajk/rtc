#include <Wire.h>
#include <rtc.h>

RTCManager rtcMgr;

void setup() {
  Serial.begin(9600);
  while (!Serial) {;}

  if (!rtcMgr.begin()) {
    Serial.println(F("RTC not found"));
    while (1);
  }

  // Optional: set to compile time on first boot if clock stopped
  rtcMgr.setCompileTimeIfStopped();
}

void loop() {
  SimpleDateTime now;
  if (rtcMgr.readNow(now)) {
    Serial.print(F("Time: "));
    Serial.print(now.year);   Serial.print('-');
    Serial.print(now.month);  Serial.print('-');
    Serial.print(now.day);    Serial.print(' ');
    Serial.print(now.hour);   Serial.print(':');
    Serial.print(now.minute); Serial.print(':');
    Serial.println(now.second);
  } else {
    Serial.println(F("RTC read failed"));
  }

  // Demo: add a minute every 30 seconds (comment out in real use)
  static uint32_t last = 0;
  if (millis() - last > 30000UL) {
    rtcMgr.adjustMinutes(1);
    last = millis();
    Serial.println(F("Adjusted +1 minute"));
  }

  delay(1000);
}

