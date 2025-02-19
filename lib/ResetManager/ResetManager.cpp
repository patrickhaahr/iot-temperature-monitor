#include "ResetManager.h"
#include <Arduino.h>

ResetManager::ResetManager(uint8_t buttonPin, unsigned long holdTime)
    : pin(buttonPin), holdTime(holdTime), buttonPressed(false), pressStartTime(0) {
}

bool ResetManager::begin() {
    pinMode(pin, INPUT_PULLUP);
    return true;
}

void ResetManager::check() {
    bool currentState = digitalRead(pin) == LOW;
    
    if (currentState && !buttonPressed) {
        // Button just pressed
        buttonPressed = true;
        pressStartTime = millis();
    }
    else if (!currentState && buttonPressed) {
        // Button released
        buttonPressed = false;
    }
    else if (buttonPressed && (millis() - pressStartTime >= holdTime)) {
        // Button held for required duration
        buttonPressed = false;
        if (resetCallback) {
            resetCallback();
        }
    }
}

void ResetManager::setResetCallback(std::function<void(void)> callback) {
    resetCallback = callback;
}
