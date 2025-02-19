#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <functional>

/**
 * @brief Manages the web server and WebSocket functionality
 * 
 * This class handles the web server setup, serves static files from SPIFFS,
 * and manages WebSocket connections for real-time temperature updates.
 */
class WebServerManager {
public:
    /**
     * @brief Construct a new Web Server Manager object
     * 
     * @param port Port number for the web server
     */
    WebServerManager(uint16_t port = 80);

    /**
     * @brief Initialize the web server
     * 
     * @return true if initialization was successful
     * @return false if initialization failed
     */
    bool begin();

    /**
     * @brief Set the callback function for WiFi credentials
     * 
     * @param callback Function to handle received WiFi credentials
     */
    void setWiFiCredentialsCallback(std::function<void(const char*, const char*)> callback);

    /**
     * @brief Broadcast temperature data to all connected WebSocket clients
     * 
     * @param temperature Current temperature reading
     */
    void broadcastTemperature(float temperature);

private:
    AsyncWebServer* server;
    AsyncWebSocket* ws;
    uint16_t port;
    std::function<void(const char*, const char*)> wifiCredentialsCallback;

    void setupRoutes();
    void handleWebSocketMessage(AsyncWebSocket* server, AsyncWebSocketClient* client, 
                              AwsFrameInfo* info, uint8_t* data, size_t len);
    void onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                         AwsEventType type, void* arg, uint8_t* data, size_t len);
};

#endif // WEB_SERVER_MANAGER_H
