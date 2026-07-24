#include "ds3231.h"

#define DS3231_ADDRESS 0x68
static RTC_DS3231 rtc;
static constexpr uint32_t FALLBACK_REBOOT_DELAY_SECONDS = 4UL * 24 * 60 * 60 + 5 * 60;

static byte decToBcd(byte val)
{
    return ((val / 10 * 16) + (val % 10));
}

static void resetAlarms()
{
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0F);  // registro Status
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 1);
    byte status = Wire.read();

    status &= ~0b00000011; // reset A1F e A2F

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0F);
    Wire.write(status);
    Wire.endTransmission();
}

static void setAlarm1(DateTime dt)
{
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x07); // primo registro di Alarm1

    Wire.write(decToBcd(dt.second()) & 0x7F); // secondi
    Wire.write(decToBcd(dt.minute()) & 0x7F); // minuti
    Wire.write(decToBcd(dt.hour())   & 0x7F); // ore
    Wire.write(decToBcd(dt.day())    & 0x7F); // giorno del mese
    Wire.endTransmission();

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0E); // registro Control
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 1);
    byte ctrl = Wire.read();
    ctrl |= 0b00000101; // INTCN and Alarm1 interrupt enable

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0E);
    Wire.write(ctrl);
    Wire.endTransmission();
}

void ds3231ScheduleFallbackReboot()
{
    if (!rtc.begin()) {
        Serial.println("Errore: DS3231 non trovato!");
        return;
    }

    DateTime now = rtc.now();

    resetAlarms();

    DateTime newAlarm = now + TimeSpan(FALLBACK_REBOOT_DELAY_SECONDS);
    Serial.println(newAlarm.timestamp());

    setAlarm1(newAlarm);
}
