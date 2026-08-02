#include "configuration.h"

#if HAS_TELEMETRY && !MESHTASTIC_EXCLUDE_ENVIRONMENTAL_SENSOR && __has_include(<Adafruit_INA219.h>)

#include "../mesh/generated/meshtastic/telemetry.pb.h"
#include "INA219Sensor.h"
#include "TelemetrySensor.h"
#include <Adafruit_INA219.h>

#ifndef INA219_MULTIPLIER
#define INA219_MULTIPLIER 1.0f
#endif

INA219Sensor::INA219Sensor(uint8_t powerChannel)
    : TelemetrySensor(meshtastic_TelemetrySensorType_INA219, powerChannel == 1 ? "INA219" : "INA219-2"),
      powerChannel(powerChannel)
{
}

void INA219Sensor::configure(uint8_t address, TwoWire *wire)
{
    this->address = address;
    this->wire = wire;
}

uint8_t INA219Sensor::getAddress() const
{
    if (address > 0) {
        return address;
    }
    return powerChannel == 1 ? nodeTelemetrySensorsMap[sensorType].first : 0;
}

bool INA219Sensor::hasSensor()
{
    return getAddress() > 0;
}

int32_t INA219Sensor::runOnce()
{
    LOG_INFO("Init sensor: %s", sensorName);
    if (!hasSensor()) {
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }
    if (!ina219.success()) {
        ina219 = Adafruit_INA219(getAddress());
        TwoWire *sensorWire = wire != nullptr ? wire : nodeTelemetrySensorsMap[sensorType].second;
        status = ina219.begin(sensorWire);
    } else {
        status = ina219.success();
    }

    if (!status) {
        LOG_WARN("Can't connect to detected %s sensor", sensorName);
        if (powerChannel == 1) {
            nodeTelemetrySensorsMap[sensorType].first = 0;
        }
        initialized = true;
        return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
    }

    LOG_INFO("Opened %s sensor on i2c bus", sensorName);
    setup();
    initialized = true;
    return DEFAULT_SENSOR_MINIMUM_WAIT_TIME_BETWEEN_READS;
}

void INA219Sensor::setup() {}

bool INA219Sensor::getMetrics(meshtastic_Telemetry *measurement)
{
    switch (measurement->which_variant) {
    case meshtastic_Telemetry_environment_metrics_tag:
        return getEnvironmentMetrics(measurement);
    case meshtastic_Telemetry_power_metrics_tag:
        return getPowerMetrics(measurement);
    default:
        return false;
    }
}

bool INA219Sensor::getEnvironmentMetrics(meshtastic_Telemetry *measurement)
{
    measurement->variant.environment_metrics.has_voltage = true;
    measurement->variant.environment_metrics.has_current = true;

    measurement->variant.environment_metrics.voltage = ina219.getBusVoltage_V();
    measurement->variant.environment_metrics.current = ina219.getCurrent_mA() * INA219_MULTIPLIER;
    return true;
}

bool INA219Sensor::getPowerMetrics(meshtastic_Telemetry *measurement)
{
    if (powerChannel == 2) {
        measurement->variant.power_metrics.has_ch2_voltage = true;
        measurement->variant.power_metrics.has_ch2_current = true;
        measurement->variant.power_metrics.ch2_voltage = ina219.getBusVoltage_V();
        measurement->variant.power_metrics.ch2_current = ina219.getCurrent_mA() * INA219_MULTIPLIER;
    } else {
        measurement->variant.power_metrics.has_ch1_voltage = true;
        measurement->variant.power_metrics.has_ch1_current = true;
        measurement->variant.power_metrics.ch1_voltage = ina219.getBusVoltage_V();
        measurement->variant.power_metrics.ch1_current = ina219.getCurrent_mA() * INA219_MULTIPLIER;
    }
    return true;
}

uint16_t INA219Sensor::getBusVoltageMv()
{
    return lround(ina219.getBusVoltage_V() * 1000);
}

int16_t INA219Sensor::getCurrentMa()
{
    return lround(ina219.getCurrent_mA());
}

#endif
