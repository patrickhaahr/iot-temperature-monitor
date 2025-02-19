#ifndef RESET_MANAGER_H
#define RESET_MANAGER_H

#include <functional>

/**
 * @brief Manages the hardware reset button functionality
 * 
 * This class handles the monitoring of a hardware button that, when held
 * for a specified duration, triggers a reset of the WiFi configuration.
 */
class ResetManager {
public:
    /**
     * @brief Construct a new Reset Manager object
     * 
     * @param buttonPin GPIO pin number where the reset button is connected
     * @param holdTime Time in milliseconds the button needs to be held for reset (default 10000ms)
     */
    ResetManager(uint8_t buttonPin, unsigned long holdTime = 10000);

    /**
     * @brief Initialize the reset manager
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin();

    /**
     * @brief Check the reset button state
     * 
     * This method should be called regularly in the main loop to check
     * if the reset button is being held.
     */
    void check();

    /**
     * @brief Set the callback function to be called when reset is triggered
     * 
     * @param callback Function to be called when reset is triggered
     */
    void setResetCallback(std::function<void(void)> callback);

private:
    uint8_t pin;
    unsigned long holdTime;
    unsigned long pressStartTime;
    bool buttonPressed;
    std::function<void(void)> resetCallback;
};

#endif // RESET_MANAGER_H
