#include "SolarChargeController.h"

#if defined(HAS_SOLAR_CHARGE_CONTROL) && HAS_SOLAR_CHARGE_CONTROL

#include "detect/ScanI2CTwoWire.h"
#include "main.h"
#include "power.h"
#include <cmath>

const char *SolarChargeController::stateName(State state)
{
    switch (state) {
    case SolarChargeController::State::WAIT_SENSORS:
        return "waiting for sensors";
    case SolarChargeController::State::WAIT_TEMPERATURE:
        return "temperature blocked";
    case SolarChargeController::State::WAIT_SUN:
        return "waiting for sun";
    case SolarChargeController::State::PROBE:
        return "probing panel";
    case SolarChargeController::State::CHARGING:
        return "charging";
    }
    return "unknown";
}

void solarChargeSetPanelConnected(bool connected)
{
    digitalWrite(SOLAR_CHARGE_CONTROL_PIN, connected ? SOLAR_CHARGE_CONTROL_ON : SOLAR_CHARGE_CONTROL_OFF);
    pinMode(SOLAR_CHARGE_CONTROL_PIN, OUTPUT);
}

void solarChargePrepareDeepSleep(bool emergencyCharge)
{
    solarChargeSetPanelConnected(emergencyCharge);
    LOG_INFO("Solar panel %s for deep sleep", emergencyCharge ? "connected" : "disconnected");
}

SolarChargeController::SolarChargeController()
    : concurrency::OSThread("SolarCharge", SOLAR_SENSOR_RETRY_INTERVAL_MS), ScanI2CConsumer()
{
    solarChargeSetPanelConnected(false);
}

void SolarChargeController::i2cScanFinished(ScanI2C *i2cScanner)
{
    solarIna219 = getINA219SensorByAddress(SOLAR_INA219_ADDRESS);
    if (solarIna219 && !solarIna219->isInitialized())
        solarIna219->runOnce();

    for (const auto &device : i2cScanner->findAll(ScanI2C::DeviceType::BMP_280)) {
        if (device.address.address == SOLAR_BMP280_ADDRESS) {
            bmpBus = ScanI2CTwoWire::fetchI2CBus(device.address);
            break;
        }
    }

    bmpReady = initializeBmp();
    if (!solarIna219)
        LOG_ERROR("Solar INA219 not found at address 0x%02x", SOLAR_INA219_ADDRESS);
    if (!bmpReady)
        LOG_ERROR("Solar BMP280 not found at address 0x%02x", SOLAR_BMP280_ADDRESS);

    setIntervalFromNow(0);
}

bool SolarChargeController::initializeBmp()
{
    if (!bmpBus)
        return false;

    bmp280 = Adafruit_BMP280(bmpBus);
    if (!bmp280.begin(SOLAR_BMP280_ADDRESS))
        return false;

    bmp280.setSampling(Adafruit_BMP280::MODE_FORCED, Adafruit_BMP280::SAMPLING_X1, Adafruit_BMP280::SAMPLING_X1,
                       Adafruit_BMP280::FILTER_OFF, Adafruit_BMP280::STANDBY_MS_1000);
    return true;
}

bool SolarChargeController::readMeasurements(float &temperatureC, uint16_t &panelVoltageMv)
{
    if (!bmpReady)
        bmpReady = initializeBmp();
    if (!solarIna219)
        solarIna219 = getINA219SensorByAddress(SOLAR_INA219_ADDRESS);
    if (solarIna219 && !solarIna219->isRunning())
        solarIna219->runOnce();
    if (!bmpReady || !solarIna219 || !solarIna219->isRunning())
        return false;

    if (!bmp280.takeForcedMeasurement())
        return false;

    temperatureC = bmp280.readTemperature();
    panelVoltageMv = solarIna219->getBusVoltageMv();
    return std::isfinite(temperatureC) && temperatureC >= -40.0F && temperatureC <= 85.0F;
}

bool SolarChargeController::temperatureAllowsStart(float temperatureC) const
{
    return temperatureC >= SOLAR_CHARGE_START_MIN_TEMP_C && temperatureC <= SOLAR_CHARGE_START_MAX_TEMP_C;
}

bool SolarChargeController::temperatureRequiresStop(float temperatureC) const
{
    return temperatureC <= SOLAR_CHARGE_STOP_MIN_TEMP_C || temperatureC >= SOLAR_CHARGE_STOP_MAX_TEMP_C;
}

void SolarChargeController::setState(State nextState)
{
    if (state == nextState)
        return;

    state = nextState;
    startSamples = 0;
    stopSamples = 0;
    LOG_INFO("Solar charge state: %s", stateName(state));
}

int32_t SolarChargeController::runOnce()
{
    float temperatureC = NAN;
    uint16_t panelVoltageMv = 0;
    if (!readMeasurements(temperatureC, panelVoltageMv)) {
        solarChargeSetPanelConnected(false);
        setState(State::WAIT_SENSORS);
        return SOLAR_SENSOR_RETRY_INTERVAL_MS;
    }

    LOG_DEBUG("Solar charge: %.1f C, panel %u mV", temperatureC, panelVoltageMv);

    if (state == State::PROBE || state == State::CHARGING) {
        if (temperatureRequiresStop(temperatureC)) {
            solarChargeSetPanelConnected(false);
            setState(State::WAIT_TEMPERATURE);
            return SOLAR_WAIT_TEMPERATURE_INTERVAL_MS;
        }
    } else if (!temperatureAllowsStart(temperatureC)) {
        solarChargeSetPanelConnected(false);
        setState(State::WAIT_TEMPERATURE);
        return SOLAR_WAIT_TEMPERATURE_INTERVAL_MS;
    }

    switch (state) {
    case State::WAIT_SENSORS:
    case State::WAIT_TEMPERATURE:
        solarChargeSetPanelConnected(false);
        setState(State::WAIT_SUN);
        return SOLAR_WAIT_SUN_INTERVAL_MS;

    case State::WAIT_SUN:
        if (panelVoltageMv < SOLAR_PANEL_START_MV) {
            startSamples = 0;
            return SOLAR_WAIT_SUN_INTERVAL_MS;
        }
        if (++startSamples < SOLAR_PANEL_START_SAMPLES)
            return 1000;

        solarChargeSetPanelConnected(true);
        setState(State::PROBE);
        return SOLAR_PANEL_PROBE_DELAY_MS;

    case State::PROBE:
        if (panelVoltageMv < SOLAR_PANEL_STOP_MV) {
            solarChargeSetPanelConnected(false);
            setState(State::WAIT_SUN);
            return SOLAR_WAIT_SUN_INTERVAL_MS;
        }
        setState(State::CHARGING);
        return SOLAR_CHARGING_CHECK_INTERVAL_MS;

    case State::CHARGING:
        if (panelVoltageMv >= SOLAR_PANEL_STOP_MV) {
            stopSamples = 0;
            return SOLAR_CHARGING_CHECK_INTERVAL_MS;
        }
        if (++stopSamples < SOLAR_PANEL_STOP_SAMPLES)
            return SOLAR_CHARGING_CHECK_INTERVAL_MS;

        solarChargeSetPanelConnected(false);
        setState(State::WAIT_SUN);
        return SOLAR_WAIT_SUN_INTERVAL_MS;
    }

    solarChargeSetPanelConnected(false);
    setState(State::WAIT_SENSORS);
    return SOLAR_SENSOR_RETRY_INTERVAL_MS;
}

#endif
