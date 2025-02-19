#include "ResetManager.h"
#include <Arduino.h>

#define RED_LED 5  // Define the RED_LED pin

ResetManager::ResetManager(uint8_t buttonPin, unsigned long holdTime)
    : pin(buttonPin), holdTime(holdTime), buttonPressed(false), pressStartTime(0) {
    button = new ezButton(buttonPin);
    button->setDebounceTime(50); // 50ms debounce
}

bool ResetManager::begin() {
    pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
    // Ensure that the button pin is configured correctly in your main code.
    return true;
}

void ResetManager::check() {
    button->loop(); // Update the button state

    // Adjust the following line based on your wiring:
    // If your button wiring gives HIGH when pressed, use this:
    bool isPressed = (button->getState() == HIGH);
    // If your wiring gives LOW when pressed, change to:
    // bool isPressed = (button->getState() == LOW);

    if (isPressed) {
        if (!buttonPressed) {
            buttonPressed = true;
            pressStartTime = millis();
            digitalWrite(RED_LED, HIGH);
            Serial.println("[RESET] Button pressed - hold for 10 seconds to reset WiFi configuration");
        }

        unsigned long elapsedTime = millis() - pressStartTime;
        static int lastReportedSecond = -1;
        int currentSecond = elapsedTime / 1000;
        if (currentSecond != lastReportedSecond) {
            lastReportedSecond = currentSecond;
            Serial.printf("[RESET] Holding for %d seconds...\n", currentSecond);
        }

        if (elapsedTime >= holdTime) {
            buttonPressed = false;
            digitalWrite(RED_LED, LOW);
            Serial.println("[RESET] 10-second hold completed - initiating reset");
            if (resetCallback) {
                resetCallback();
            }
        }
    } else {  // Button is not pressed
        if (buttonPressed) {
            buttonPressed = false;
            pressStartTime = 0;
            digitalWrite(RED_LED, LOW);
            Serial.println("[RESET] Button released - reset cancelled");
        }
    }
}


void ResetManager::setResetCallback(std::function<void()> callback) {
    resetCallback = callback;
}
