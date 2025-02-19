#include "ResetManager.h"
#include <Arduino.h>

ResetManager::ResetManager(uint8_t buttonPin, unsigned long holdTime)
    : pin(buttonPin), holdTime(holdTime), buttonPressed(false), pressStartTime(0) {
}

bool ResetManager::begin() {
    pinMode(pin, INPUT_PULLUP);
    // Give time for the pull-up to stabilize
    delay(10);
    return true;
}

void ResetManager::check() {
    static unsigned long lastDebounceTime = 0;
    static bool lastButtonState = HIGH;
    const unsigned long debounceDelay = 50;  // 50ms debounce time
    
    // Read the current state with debouncing
    bool reading = digitalRead(pin);
    
    if (reading != lastButtonState) {
        lastDebounceTime = millis();
    }
    
    if ((millis() - lastDebounceTime) > debounceDelay) {
        bool currentState = reading == LOW;  // Button is pressed when LOW (due to pull-up)
        
        if (currentState && !buttonPressed) {
            // Button just pressed
            buttonPressed = true;
            pressStartTime = millis();
            Serial.println("Reset button pressed");
        }
        else if (!currentState && buttonPressed) {
            // Button released
            buttonPressed = false;
            Serial.println("Reset button released");
        }
        else if (buttonPressed && (millis() - pressStartTime >= holdTime)) {
            // Button held for required duration
            buttonPressed = false;
            if (resetCallback) {
                Serial.println("Reset button held for required duration - triggering reset");
                resetCallback();
            }
        }
    }
    
    lastButtonState = reading;
}

void ResetManager::setResetCallback(std::function<void(void)> callback) {
    resetCallback = callback;
}
