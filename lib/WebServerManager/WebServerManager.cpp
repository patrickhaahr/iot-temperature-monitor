#include "WebServerManager.h"
#include <ESPAsyncWebServer.h>
#include "SensorManager.h"
#include <ArduinoJson.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type,
               void * arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.println("WebSocket client connected");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.println("WebSocket client disconnected");
  }
}

void setupWebServer() {
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("Web server started.");
}

WebServerManager::WebServerManager(uint16_t port) : port(port), isInAPMode(false) {
    server = new AsyncWebServer(port);
    ws = new AsyncWebSocket("/ws");
}

bool WebServerManager::begin() {
    if (!SPIFFS.begin(true)) {
        return false;
    }

    // Attach WebSocket handler
    ws->onEvent([this](AsyncWebSocket* server, AsyncWebSocketClient* client,
                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
        onWebSocketEvent(server, client, type, arg, data, len);
    });
    server->addHandler(ws);

    setupRoutes();
    server->begin();
    return true;
}

void WebServerManager::setupRoutes() {
    // Serve static files with AP mode check
    server->on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        if (isInAPMode) {
            // In AP mode, serve the configuration page
            request->send(SPIFFS, "/config.html");
        } else {
            // In connected mode, serve the temperature monitor
            request->send(SPIFFS, "/index.html");
        }
    });

    // Serve other static files
    server->serveStatic("/", SPIFFS, "/");

    // Handle WiFi configuration in AP mode
    server->on("/api/wifi/configure", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(400); // Bad request by default
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        if (!isInAPMode) {
            request->send(403, "application/json", "{\"status\":\"error\",\"message\":\"Not in AP mode\"}");
            return;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        
        if (!error && doc.containsKey("ssid") && doc.containsKey("password")) {
            const char* ssid = doc["ssid"];
            const char* password = doc["password"];
            
            if (wifiCredentialsCallback) {
                wifiCredentialsCallback(ssid, password);
            }
            
            request->send(200, "application/json", "{\"status\":\"success\"}");
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request\"}");
        }
    });

    // Add an endpoint to check the current mode
    server->on("/api/mode", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["mode"] = isInAPMode ? "ap" : "connected";
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    // Export temperature data
    server->on("/api/data/export", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists("/temperature_log.json")) {
            request->send(404, "application/json", "{\"status\":\"error\",\"message\":\"No data found\"}");
            return;
        }
        request->send(SPIFFS, "/temperature_log.json", "application/json");
    });

    // Reset WiFi configuration
    server->on("/api/system/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        if (systemResetCallback) {
            systemResetCallback();
            request->send(200, "application/json", "{\"status\":\"success\",\"message\":\"System will reset\"}");
        } else {
            request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Reset handler not configured\"}");
        }
    });

    // Update system settings
    server->on("/api/system/settings", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(400); // Bad request by default
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, data);
        
        if (!error && doc.containsKey("loggingInterval") && 
            doc.containsKey("tempUpdateInterval") && 
            doc.containsKey("maxLogEntries")) {
            
            int loggingInterval = doc["loggingInterval"];
            int tempUpdateInterval = doc["tempUpdateInterval"];
            int maxLogEntries = doc["maxLogEntries"];
            
            if (systemSettingsCallback) {
                systemSettingsCallback(loggingInterval, tempUpdateInterval, maxLogEntries);
                request->send(200, "application/json", "{\"status\":\"success\"}");
            } else {
                request->send(500, "application/json", "{\"status\":\"error\",\"message\":\"Settings handler not configured\"}");
            }
        } else {
            request->send(400, "application/json", "{\"status\":\"error\",\"message\":\"Invalid request\"}");
        }
    });

    // Get current system settings
    server->on("/api/system/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        JsonDocument doc;
        File file = SPIFFS.open("/settings.json", "r");
        if (!file) {
            doc["loggingInterval"] = 300; // Default 5 minutes
            doc["tempUpdateInterval"] = 5; // Default 5 seconds
            doc["maxLogEntries"] = 1000;  // Default 1000 entries
        } else {
            deserializeJson(doc, file);
            file.close();
        }
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });

    server->on("/api/temperature/history", HTTP_GET, [this](AsyncWebServerRequest *request) {
        if (!SPIFFS.exists("/temperature_log.json")) {
            request->send(404, "application/json", "{\"error\":\"No history found\"}");
            return;
        }
        
        request->send(SPIFFS, "/temperature_log.json", "application/json");
    });
}

void WebServerManager::setWiFiCredentialsCallback(std::function<void(const char*, const char*)> callback) {
    wifiCredentialsCallback = callback;
}

void WebServerManager::setSystemResetCallback(std::function<void(void)> callback) {
    systemResetCallback = callback;
}

void WebServerManager::setSystemSettingsCallback(std::function<void(int, int, int)> callback) {
    systemSettingsCallback = callback;
}

void WebServerManager::broadcastTemperature(float temperature) {
    if (ws->count() > 0) {
        time_t now;
        time(&now);
        
        // Format time string to match JSON log format
        struct tm timeinfo;
        localtime_r(&now, &timeinfo);
        char timeStr[20];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
        
        // Send both the new reading and update trigger
        String jsonString = "{\"update\":true,\"temperature\":";
        jsonString += String(temperature, 4);  // More precision
        jsonString += ",\"timestamp\":\"";
        jsonString += timeStr;
        jsonString += "\"}";
        
        ws->textAll(jsonString);
    }
}

void WebServerManager::handleWebSocketMessage(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                            AwsFrameInfo* info, uint8_t* data, size_t len) {
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
    }
}

void WebServerManager::onWebSocketEvent(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                      AwsEventType type, void* arg, uint8_t* data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            // Client connected
            break;
        case WS_EVT_DISCONNECT:
            // Client disconnected
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(server, client, (AwsFrameInfo*)arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void WebServerManager::setAPMode(bool isAP) {
    isInAPMode = isAP;
}
