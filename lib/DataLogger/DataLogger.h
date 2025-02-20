#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

class DataLogger {
public:
    /**
     * @brief Construct a new Data Logger object
     * 
     * @param logFileName Name of the file to store temperature logs
     * @param loggingIntervalSeconds Interval between temperature readings in seconds
     */
    DataLogger(const char* logFileName = "/temperature_log.json", 
               unsigned long loggingIntervalSeconds = 300);  // 300 seconds = 5 minutes

    /**
     * @brief Initialize the data logger
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin();

    /**
     * @brief Log a temperature reading with current timestamp
     * 
     * @param temperature Temperature value in Celsius
     * @return true if logging was successful
     * @return false if logging failed
     */
    bool logTemperature(float temperature);

    /**
     * @brief Check if it's time to log a new reading
     * 
     * @return true if logging interval has elapsed
     * @return false if it's not time to log yet
     */
    bool shouldLog();

    /**
     * @brief Get the last logged temperature
     * 
     * @return float Last logged temperature value
     */
    float getLastTemperature() const { return lastTemperature; }

    /**
     * @brief Get the last logging time
     * 
     * @return time_t Last logging timestamp
     */
    time_t getLastLogTime() const { return lastLogTime; }

private:
    const char* filename;
    unsigned long intervalSeconds;
    time_t lastLogTime;
    float lastTemperature;

    /**
     * @brief Create a new log file with initial structure
     * 
     * @return true if file creation was successful
     * @return false if file creation failed
     */
    bool createLogFile();

    /**
     * @brief Append a temperature reading to the log file
     * 
     * @param temperature Temperature value to log
     * @param timestamp Unix timestamp of the reading
     * @return true if append was successful
     * @return false if append failed
     */
    bool appendToLog(float temperature, time_t timestamp);
};

#endif // DATA_LOGGER_H 