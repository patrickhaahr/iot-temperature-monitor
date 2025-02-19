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
    
    // Wait for first conversion (required for first reading)
    sensors->requestTemperatures();
    delay(750);
    
    // Check if we can get a valid reading
    float temp = sensors->getTempCByIndex(0);
    isInitialized = (temp != DEVICE_DISCONNECTED_C);
    
    return isInitialized;
}

float SensorManager::getTemperature() {
    if (!isInitialized) {
        Serial.println("Sensor not initialized!");
        return DEVICE_DISCONNECTED_C;
    }
    
    sensors->requestTemperatures();
    float temp = sensors->getTempCByIndex(0);
    
    if (temp == DEVICE_DISCONNECTED_C) {
        Serial.println("Error: Sensor disconnected!");
    }
    
    return temp;
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
