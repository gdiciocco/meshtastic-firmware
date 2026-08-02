#pragma once

#include "configuration.h"

#if defined(HAS_SOLAR_CHARGE_CONTROL) && HAS_SOLAR_CHARGE_CONTROL

#include "concurrency/OSThread.h"
#include "detect/ScanI2CConsumer.h"
#include "modules/Telemetry/Sensor/INA219Sensor.h"
#include <Adafruit_BMP280.h>

class SolarChargeController : public concurrency::OSThread, public ScanI2CConsumer
{
  public:
    SolarChargeController();
    void i2cScanFinished(ScanI2C *i2cScanner) override;

  protected:
    int32_t runOnce() override;

  private:
    enum class State : uint8_t { WAIT_SENSORS, WAIT_TEMPERATURE, WAIT_SUN, PROBE, CHARGING };

    State state = State::WAIT_SENSORS;
    Adafruit_BMP280 bmp280;
    TwoWire *bmpBus = nullptr;
    INA219Sensor *solarIna219 = nullptr;
    uint8_t startSamples = 0;
    uint8_t stopSamples = 0;
    bool bmpReady = false;

    bool initializeBmp();
    bool readMeasurements(float &temperatureC, uint16_t &panelVoltageMv);
    bool temperatureAllowsStart(float temperatureC) const;
    bool temperatureRequiresStop(float temperatureC) const;
    static const char *stateName(State state);
    void setState(State nextState);
};

void solarChargeSetPanelConnected(bool connected);
void solarChargePrepareDeepSleep(bool emergencyCharge);

#endif
