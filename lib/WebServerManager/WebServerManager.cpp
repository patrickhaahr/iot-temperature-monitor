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

WebServerManager::WebServerManager(uint16_t port) : port(port) {
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
    // Serve static files
    server->serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Handle WiFi configuration in AP mode
    server->on("/api/wifi/configure", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(400); // Bad request by default
    }, NULL, [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
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
}

void WebServerManager::setWiFiCredentialsCallback(std::function<void(const char*, const char*)> callback) {
    wifiCredentialsCallback = callback;
}

void WebServerManager::broadcastTemperature(float temperature) {
    if (ws->count() > 0) {
        JsonDocument doc;
        doc["type"] = "temperature";
        doc["value"] = temperature;
        
        String jsonString;
        serializeJson(doc, jsonString);
        ws->textAll(jsonString);
    }
}

void WebServerManager::handleWebSocketMessage(AsyncWebSocket* server, AsyncWebSocketClient* client,
                                            AwsFrameInfo* info, uint8_t* data, size_t len) {
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        // Handle WebSocket messages if needed
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
