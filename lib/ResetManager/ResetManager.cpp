#include "ResetManager.h"
#include <Arduino.h>

#define RED_LED 5    // Red LED pin
#define BLUE_LED 18  // Blue LED pin

ResetManager::ResetManager(uint8_t buttonPin, unsigned long holdTime)
    : pin(buttonPin), holdTime(holdTime), buttonPressed(false), pressStartTime(0) {
    button = new ezButton(buttonPin);
    button->setDebounceTime(50); // 50ms debounce
}

bool ResetManager::begin() {
    pinMode(RED_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(BLUE_LED, LOW);
    // Ensure that the button pin is configured appropriately in your main code.
    return true;
}

void ResetManager::check() {
    button->loop(); // Update the button state

    // Adjust this condition based on your wiring.
    // If your wiring gives HIGH when pressed, use the following:
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

            // Blink blue LED rapidly for 1 second (50ms on, 50ms off for 10 cycles)
            for (int i = 0; i < 10; i++) {
                digitalWrite(BLUE_LED, HIGH);
                delay(50);
                digitalWrite(BLUE_LED, LOW);
                delay(50);
            }

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
