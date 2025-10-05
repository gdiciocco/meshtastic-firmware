#include "ds3231.h"

#define DS3231_ADDRESS 0x68
static RTC_DS3231 rtc;

static byte decToBcd(byte val) {
    return ((val / 10 * 16) + (val % 10));
}

// reset flag allarmi
static void resetAlarms() {
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

// imposta Alarm1 su data/ora
static void setAlarm1(DateTime dt) {
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x07); // primo registro di Alarm1

    Wire.write(decToBcd(dt.second()) & 0x7F); // secondi
    Wire.write(decToBcd(dt.minute()) & 0x7F); // minuti
    Wire.write(decToBcd(dt.hour())   & 0x7F); // ore
    Wire.write(decToBcd(dt.day())    & 0x7F); // giorno del mese
    Wire.endTransmission();

    // abilito Alarm1 (A1IE=1)
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0E); // registro Control
    Wire.endTransmission();

    Wire.requestFrom(DS3231_ADDRESS, 1);
    byte ctrl = Wire.read();
    ctrl |= 0b00000001;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(0x0E);
    Wire.write(ctrl);
    Wire.endTransmission();
}

void ds3231_set_alarm_in_24h() {
    if (!rtc.begin()) {
        Serial.println("Errore: DS3231 non trovato!");
        return;
    }

    DateTime now = rtc.now();

    resetAlarms();

    DateTime newAlarm = now + TimeSpan(4, 0, 0, 0);
    Serial.println(newAlarm.timestamp());

    setAlarm1(newAlarm);
}
