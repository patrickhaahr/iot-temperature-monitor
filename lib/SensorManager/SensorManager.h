#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <OneWire.h>
#include <DallasTemperature.h>

/**
 * @brief Manages the DS18B20 temperature sensor operations
 * 
 * This class handles the initialization, reading, and management of the DS18B20
 * temperature sensor. It provides methods to get temperature readings and check
 * sensor status.
 */
class SensorManager {
public:
    /**
     * @brief Construct a new Sensor Manager object
     * 
     * @param oneWirePin GPIO pin number where the DS18B20 sensor is connected
     */
    SensorManager(uint8_t oneWirePin);

    /**
     * @brief Initialize the temperature sensor
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin();

    /**
     * @brief Get the current temperature reading
     * 
     * @return float Temperature in Celsius
     */
    float getTemperature();

    /**
     * @brief Check if the sensor is properly connected and functioning
     * 
     * @return true if sensor is working
     * @return false if sensor is not responding
     */
    bool isSensorWorking();

private:
    OneWire* oneWire;
    DallasTemperature* sensors;
    uint8_t pin;
    bool isInitialized;
    float lastTemperature;  // Cache for the last valid temperature reading
    unsigned long lastReadTime;  // Timestamp of the last temperature reading
};

#endif // SENSOR_MANAGER_H
