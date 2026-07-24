#ifndef _VARIANT_PROMICRO_DIY_
#define _VARIANT_PROMICRO_DIY_

/** Master clock frequency */
#define VARIANT_MCK (64000000ul)

// #define USE_LFXO // Board uses 32khz crystal for LF
#define USE_LFRC // Board uses RC for LF

#define PROMICRO_DIY_TCXO
#define HAS_DS3231_WATCHDOG

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*
NRF52 PRO MICRO PIN ASSIGNMENT

| Pin   | Function    |     | Pin      | Function     | RF95  |
| ----- | ----------- | --- | -------- | ------------ | ----- |
| Gnd   |             |     | vbat     |              |       |
| P0.06 | Serial2 RX  |     | vbat     |              |       |
| P0.08 | Serial2 TX  |     | Gnd      |              |       |
| Gnd   |             |     | reset    |              |       |
| Gnd   |             |     | ext_vcc  | *see 0.13    |       |
| P0.17 | RXEN        |     | P0.31    | BATTERY_PIN  |       |
| P0.20 | GPS_TX      |     | P0.29    | BUSY         | DIO0  |
| P0.22 | GPS_RX      |     | P0.02    | MISO         | MISO  |
| P0.24 | GPS_EN      |     | P1.15    | MOSI         | MOSI  |
| P1.00 | BUTTON_PIN  |     | P1.13    | CS           | CS    |
| P0.11 | SCL         |     | P1.11    | SCK          | SCK   |
| P1.04 | SDA         |     | P0.10    | DIO1/IRQ     | DIO1  |
| P1.06 | Free pin    |     | P0.09    | RESET        | RST   |
|       |             |     |          |              |       |
|       | Mid board   |     |          | Internal     |       |
| P1.01 | Free pin    |     | 0.15     | LED          |       |
| P1.02 | Free pin    |     | 0.13     | 3V3_EN       |       |
| P1.07 | Free pin    |     |          |              |       |
*/

// Number of pins defined in PinDescription array
#define PINS_COUNT (48)
#define NUM_DIGITAL_PINS (48)
#define NUM_ANALOG_INPUTS (1)
#define NUM_ANALOG_OUTPUTS (0)

// Pin 13 enables 3.3V periphery. If the Lora module is on this pin, then it should stay enabled at all times.
#define PIN_3V3_EN (0 + 13) // P0.13

// Analog pins
#define BATTERY_PIN (0 + 31) // P0.31 Battery ADC
#define ADC_CHANNEL ADC1_GPIO4_CHANNEL
#define ADC_RESOLUTION 14
#define BATTERY_SENSE_RESOLUTION_BITS 12
#define BATTERY_SENSE_RESOLUTION 4096.0

// Voltage divider: 470kΩ + 470kΩ (equal resistors)
// Vout = Vin * (R2 / (R1 + R2)) = Vin * 0.5
#define VBAT_DIVIDER (0.5F)

// ADC voltage per LSB with 3.0V reference and 12-bit resolution
#define VBAT_MV_PER_LSB (0.73242188F)

// Compensation factor: inverse of divider ratio
#define VBAT_DIVIDER_COMP (2.0F)

// Real milliVolt per LSB accounting for voltage divider
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

#undef AREF_VOLTAGE
#define AREF_VOLTAGE 3.0
#define VBAT_AR_INTERNAL AR_INTERNAL_3_0
#define ADC_MULTIPLIER VBAT_DIVIDER_COMP
#define VBAT_RAW_TO_SCALED(x) (REAL_VBAT_MV_PER_LSB * x)

// Solar/unattended operation: stop at 3.4V instead of 3.1V.
// The last OCV point is the "0%" voltage and is what Power.cpp uses to trigger the low battery deep sleep.
// Shutting down at 3.4V keeps the LDO out of dropout and leaves enough charge in the cell to boot again
// on the next sunny day, instead of collapsing into a brownout loop the node cannot recover from.
#define OCV_ARRAY 4190, 4050, 3990, 3890, 3800, 3720, 3630, 3530, 3480, 3440, 3400

// Wake from System OFF when the battery recovers (LPCOMP).
// BATTERY_PIN is P0.31 (the pin map of this variant is 1:1 with the nRF52840 port/pin numbering),
// and P0.31 is AIN7 on the nRF52840.
#define BATTERY_LPCOMP_INPUT NRF_LPCOMP_INPUT_7
// LPCOMP compares the input against a fraction of VDD, so the fraction has to be picked against the
// divider ratio of this board: 470k/470k => V_AIN7 = VBAT * 0.5 (ADC_MULTIPLIER 2.0).
// With VDD = 3.3V, VBAT at the rising edge = (fraction * 3.3V) / 0.5:
//   4/8  = 0.5000 -> equal to the divider ratio, the crossing is undefined (discarded)
//   9/16 = 0.5625 -> V_AIN7 = 1.86V -> VBAT = 3.71V (chosen: the panel has recharged a usable amount)
//   5/8  = 0.6250 -> V_AIN7 = 2.06V -> VBAT = 4.13V (discarded: only wakes on an almost full cell)
// The fraction must also stay above the divider ratio: while the regulator is in dropout VDD tracks VBAT,
// so V_AIN7 (0.5 * VBAT) stays below the reference and no spurious rising edge can be detected. The wake
// event can only happen once VDD is regulated at 3.3V again and VBAT has actually climbed back to ~3.7V.
#define BATTERY_LPCOMP_THRESHOLD NRF_LPCOMP_REF_SUPPLY_9_16

// WIRE IC AND IIC PINS
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA (0 + 10) // P1.04
#define PIN_WIRE_SCL (0 + 9) // P0.11

// LED
#define PIN_LED1 (0 + 15) // P0.15
// Actually red
#define LED_BLUE PIN_LED1
#define LED_STATE_ON 1 // State when LED is lit

// Button
#define BUTTON_PIN (-1) // P1.00

// GPS
//#define GPS_TX_PIN (-1) // P0.22
//#define GPS_RX_PIN (-1) // P0.20

//#define PIN_GPS_EN (-1) // P0.24
//#define GPS_POWER_TOGGLE
//#define GPS_UBLOX
// define GPS_DEBUG

// UART interfaces
#define PIN_SERIAL1_TX (-1)
#define PIN_SERIAL1_RX (-1)

#define PIN_SERIAL2_RX (-1) // P0.06
#define PIN_SERIAL2_TX (-1) // P0.08

// Serial interfaces
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO (0 + 20)   // P0.02
#define PIN_SPI_MOSI (0 + 22) // P1.15
#define PIN_SPI_SCK (0 + 24)  // P1.11

#define LORA_MISO PIN_SPI_MISO
#define LORA_MOSI PIN_SPI_MOSI
#define LORA_SCK PIN_SPI_SCK
#define LORA_CS (32 + 0) // P1.13

// LORA MODULES
//#define USE_LLCC68
#define USE_SX1262
//#define USE_RF95
//#define USE_SX1268
//#define USE_LR1121

#define LORA_DIO1 (0 + 6) // P0.10 IRQ
// SX126X CONFIG
#define SX126X_CS LORA_CS      // P1.13 FIXME - we really should define LORA_CS instead
#define SX126X_DIO1 LORA_DIO1     // P0.10 IRQ
//#define SX126X_DIO2 (0 + 31)    // P0.02 AS RF SWITCH
#define SX126X_DIO2_AS_RF_SWITCH // Note for E22 modules: DIO2 is not attached internally to TXEN for automatic TX/RX switching,
                                 // so it needs connecting externally if it is used in this way
#define SX126X_BUSY (0 + 8)     // P0.29
#define SX126X_RESET (0 + 17)     // P0.09
#define SX126X_TXEN RADIOLIB_NC
#define SX126X_RXEN RADIOLIB_NC

#define SX126X_POWER_EN (0 + 2) // P0.29

//#define TCXO_OPTIONAL
#define SX126X_DIO3_TCXO_VOLTAGE 1.8



// #define SX126X_MAX_POWER 8 set this if using a high-power board!

/*
On the SX1262, DIO3 sets the voltage for an external TCXO, if one is present. If one is not present, use TCXO_OPTIONAL to try both
settings.

| Mfr          | Module           | TCXO | RF Switch | Notes                                 |
| ------------ | ---------------- | ---- | --------- | ------------------------------------- |
| Ebyte        | E22-900M22S      | Yes  | Ext       |                                       |
| Ebyte        | E22-900MM22S     | No   | Ext       |                                       |
| Ebyte        | E22-900M30S      | Yes  | Ext       |                                       |
| Ebyte        | E22-900M33S      | Yes  | Ext       | MAX_POWER must be set to 8 for this   |
| Ebyte        | E220-900M22S     | No   | Ext       | LLCC68, looks like DIO3 not connected |
| AI-Thinker   | RA-01SH          | No   | Int       | SX1262                                |
| Heltec       | HT-RA62          | Yes  | Int       |                                       |
| NiceRF       | Lora1262         | yes  | Int       |                                       |
| Waveshare    | Core1262-HF      | yes  | Ext       |                                       |
| Waveshare    | LoRa Node Module | yes  | Int       |                                       |
| Seeed        | Wio-SX1262       | yes  | Ext       | Cute! DIO2/TXEN are not exposed       |
| Seeed        | Wio-LR1121       | yes  | Int       | LR1121, needs alternate rfswitch.h    |
| AI-Thinker   | RA-02            | No   | Int       | SX1278 **433mhz band only**           |
| RF Solutions | RFM95            | No   | Int       | Untested                              |
| Ebyte        | E80-900M2213S    | Yes  | Int       | LR1121 radio                          |

*/


#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
