#ifndef DS3231_HELPER_H
#define DS3231_HELPER_H

#include <Arduino.h>
#include <Wire.h>
#include "RTClib.h"

// inizializza DS3231 e programma un allarme tra 24h
void ds3231_set_alarm_in_24h();

#endif
