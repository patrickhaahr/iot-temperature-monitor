#include "DataLogger.h"

DataLogger::DataLogger(const char* logFileName, unsigned long loggingIntervalSeconds)
    : filename(logFileName)
    , intervalSeconds(loggingIntervalSeconds)
    , lastLogTime(0)
    , lastTemperature(0.0f)
{
}

bool DataLogger::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }

    // Check if log file exists, if not create it
    if (!SPIFFS.exists(filename)) {
        return createLogFile();
    }

    return true;
}

bool DataLogger::createLogFile() {
    File file = SPIFFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to create log file");
        return false;
    }

    // Create initial JSON structure with an empty array
    String initialJson = "{\"readings\":[]}";
    if (file.print(initialJson) == 0) {
        Serial.println("Failed to write initial JSON structure");
        file.close();
        return false;
    }

    file.close();
    Serial.println("Created new log file with initial structure");
    return true;
}

bool DataLogger::logTemperature(float temperature) {
    time_t now;
    time(&now);  // Get current timestamp
    
    lastTemperature = temperature;
    lastLogTime = now;

    return appendToLog(temperature, now);
}

bool DataLogger::appendToLog(float temperature, time_t timestamp) {
    String jsonData;
    JsonDocument doc;

    // Read existing file content
    File file = SPIFFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open log file for reading, creating new file");
        file.close();
        if (!createLogFile()) {
            return false;
        }
        file = SPIFFS.open(filename, "r");
        if (!file) {
            return false;
        }
    }

    // Read the file content into a String
    while (file.available()) {
        jsonData += (char)file.read();
    }
    file.close();

    // Parse existing JSON
    DeserializationError error = deserializeJson(doc, jsonData);
    if (error) {
        Serial.print("Failed to parse log file: ");
        Serial.println(error.c_str());
        // If parsing fails, create a new file
        if (!createLogFile()) {
            return false;
        }
        // Initialize new document with empty readings array
        doc.clear();
        doc["readings"] = JsonArray();
    }

    // Ensure readings array exists
    if (!doc.containsKey("readings") || doc["readings"].isNull()) {
        doc["readings"] = JsonArray();
    }

    // Format time string
    struct tm timeinfo;
    localtime_r(&timestamp, &timeinfo);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);

    // Add new reading
    JsonArray readings = doc["readings"];
    JsonObject reading = readings.add<JsonObject>();
    reading["timestamp"] = timeStr;  // Store only the formatted time string
    reading["temperature"] = temperature;

    // Trim old readings if necessary (keep last 1000 readings)
    while (readings.size() > 1000) {
        readings.remove(0);
    }

    // Write updated JSON back to file
    file = SPIFFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to open log file for writing");
        return false;
    }

    // Serialize with pretty printing for easier debugging
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to log file");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool DataLogger::shouldLog() {
    time_t now;
    time(&now);
    return (now - lastLogTime) >= intervalSeconds;
} 