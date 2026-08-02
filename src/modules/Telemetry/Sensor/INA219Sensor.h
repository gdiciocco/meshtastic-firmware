#pragma once

#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && __has_include(<Adafruit_INA219.h>)

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "CurrentSensor.h"
#include "TelemetrySensor.h"
#include "VoltageSensor.h"
#include <Adafruit_INA219.h>

class INA219Sensor : public TelemetrySensor, VoltageSensor, CurrentSensor
{
  private:
    Adafruit_INA219 ina219;
    uint8_t address = 0;
    TwoWire *wire = nullptr;
    uint8_t powerChannel;

    bool getEnvironmentMetrics(meshtastic_Telemetry *measurement);
    bool getPowerMetrics(meshtastic_Telemetry *measurement);

  protected:
    virtual void setup() override;

  public:
    explicit INA219Sensor(uint8_t powerChannel = 1);
    void configure(uint8_t address, TwoWire *wire);
    uint8_t getAddress() const;
    bool hasSensor();
    virtual int32_t runOnce() override;
    virtual bool getMetrics(meshtastic_Telemetry *measurement) override;
    virtual uint16_t getBusVoltageMv() override;
    virtual int16_t getCurrentMa() override;
};

#endif
