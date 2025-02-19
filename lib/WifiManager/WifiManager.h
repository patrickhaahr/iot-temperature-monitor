#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

/**
 * @brief Manages WiFi connectivity and configuration
 * 
 * This class handles WiFi connection management, including reading/writing
 * credentials to SPIFFS, setting up AP mode when needed, and maintaining
 * the WiFi connection.
 */
class WifiManager {
public:
    /**
     * @brief Construct a new Wifi Manager object
     * 
     * @param apSSID SSID to use when in AP mode
     * @param apPassword Password to use when in AP mode
     */
    WifiManager(const char* apSSID = "ESP32_Config", const char* apPassword = "12345678");

    /**
     * @brief Initialize the WiFi manager
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin();

    /**
     * @brief Connect to WiFi using stored credentials
     * 
     * @return true if connection successful
     * @return false if connection failed
     */
    bool connect();

    /**
     * @brief Start AP mode for configuration
     */
    void startAPMode();

    /**
     * @brief Save WiFi credentials to SPIFFS
     * 
     * @param ssid WiFi SSID
     * @param password WiFi password
     * @return true if save was successful
     * @return false if save failed
     */
    bool saveCredentials(const char* ssid, const char* password);

    /**
     * @brief Delete stored WiFi credentials
     * 
     * @return true if deletion was successful
     * @return false if deletion failed
     */
    bool deleteCredentials();

    /**
     * @brief Check if device is connected to WiFi
     * 
     * @return true if connected
     * @return false if not connected
     */
    bool isConnected();

private:
    const char* apSSID;
    const char* apPassword;
    bool loadCredentials(String& ssid, String& password);
    static const char* CONFIG_FILE;
};

#endif // WIFI_MANAGER_H
