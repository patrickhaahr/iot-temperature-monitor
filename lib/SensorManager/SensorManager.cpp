#include "SensorManager.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 19

SensorManager::SensorManager(uint8_t oneWirePin) : pin(oneWirePin), isInitialized(false) {
    oneWire = new OneWire(pin);
    sensors = new DallasTemperature(oneWire);
}

bool SensorManager::begin() {
    sensors->begin();
    
    // Wait for first conversion
    sensors->requestTemperatures();
    delay(750);
    
    // Initialize cache
    float temp = sensors->getTempCByIndex(0);
    isInitialized = (temp != DEVICE_DISCONNECTED_C);
    if (isInitialized) {
        lastTemperature = temp;
        lastReadTime = millis();
    }
    
    return isInitialized;
}

float SensorManager::getTemperature() {
    if (!isInitialized) {
        Serial.println("Sensor not initialized!");
        return DEVICE_DISCONNECTED_C;
    }
    
    // Only request new temperature every 1 second
    unsigned long now = millis();
    if (now - lastReadTime >= 1000) {
        sensors->requestTemperatures();
        float temp = sensors->getTempCByIndex(0);
        
        if (temp == DEVICE_DISCONNECTED_C) {
            Serial.println("Error: Sensor disconnected!");
        } else {
            lastTemperature = temp;
            lastReadTime = now;
        }
    }
    
    return lastTemperature;
}

bool SensorManager::isSensorWorking() {
    if (!isInitialized) {
        Serial.println("Sensor not initialized!");
        return false;
    }
    
    sensors->requestTemperatures();
    float temp = sensors->getTempCByIndex(0);
    bool working = (temp != DEVICE_DISCONNECTED_C);
    
    if (!working) {
        Serial.println("Error: Sensor not working!");
    }
    
    return working;
}
