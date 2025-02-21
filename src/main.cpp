#include <Arduino.h>
#include "SensorManager.h"
#include "WifiManager.h"
#include "WebServerManager.h"
#include "ResetManager.h"
#include "DataLogger.h"
#include <time.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // GMT+1 (3600 seconds)
const int daylightOffset_sec = 3600;  // 1 hour DST

// Pin Definitions
#define TEMPERATURE_SENSOR 19    // DS18B20 data pin
#define RESET_BUTTON       4     // Reset button pin
#define RED_LED           5     // Red LED pin
#define BLUE_LED          18    // Blue LED pin

// Global objects
SensorManager* sensorManager;
WifiManager* wifiManager;
WebServerManager* webServerManager;
ResetManager* resetManager;
DataLogger* dataLogger;

// Timer for temperature updates
unsigned long lastTempUpdate = 0;
unsigned long TEMP_UPDATE_INTERVAL = 5000; // 5 seconds

// SPIFFS status
bool spiffsInitialized = false;

// System settings
struct SystemSettings {
    int loggingInterval;
    int tempUpdateInterval;
    int maxLogEntries;
} settings;

// Function declarations
bool initializeSPIFFS();
void loadSettings();
void saveSettings();
void handleSystemSettings(int loggingInterval, int tempUpdateInterval, int maxLogEntries);

bool initializeSPIFFS() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS. Trying to format...");
        if (!SPIFFS.format()) {
            Serial.println("Failed to format SPIFFS");
            return false;
        }
        if (!SPIFFS.begin(true)) {
            Serial.println("Failed to mount SPIFFS after formatting");
            return false;
        }
    }
    Serial.println("SPIFFS mounted successfully");
    spiffsInitialized = true;
    return true;
}

void loadSettings() {
    if (!spiffsInitialized) {
        Serial.println("Cannot load settings - SPIFFS not initialized");
        // Use default settings
        settings.loggingInterval = 300;    // 5 minutes
        settings.tempUpdateInterval = 5;    // 5 seconds
        settings.maxLogEntries = 1000;     // 1000 entries
        return;
    }

    File file = SPIFFS.open("/settings.json", "r");
    if (!file) {
        // Default settings
        settings.loggingInterval = 300;    // 5 minutes
        settings.tempUpdateInterval = 5;    // 5 seconds
        settings.maxLogEntries = 1000;     // 1000 entries
        saveSettings();
        return;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        Serial.println("Failed to read settings file");
        return;
    }

    settings.loggingInterval = doc["loggingInterval"] | 300;
    settings.tempUpdateInterval = doc["tempUpdateInterval"] | 5;
    settings.maxLogEntries = doc["maxLogEntries"] | 1000;

    // Update intervals
    TEMP_UPDATE_INTERVAL = settings.tempUpdateInterval * 1000;
}

void saveSettings() {
    if (!spiffsInitialized) {
        Serial.println("Cannot save settings - SPIFFS not initialized");
        return;
    }

    StaticJsonDocument<512> doc;
    doc["loggingInterval"] = settings.loggingInterval;
    doc["tempUpdateInterval"] = settings.tempUpdateInterval;
    doc["maxLogEntries"] = settings.maxLogEntries;

    File file = SPIFFS.open("/settings.json", "w");
    if (!file) {
        Serial.println("Failed to create settings file");
        return;
    }

    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write settings file");
    }
    file.close();
}

void handleSystemSettings(int loggingInterval, int tempUpdateInterval, int maxLogEntries) {
    settings.loggingInterval = loggingInterval;
    settings.tempUpdateInterval = tempUpdateInterval;
    settings.maxLogEntries = maxLogEntries;
    
    saveSettings();
    
    // Update intervals
    TEMP_UPDATE_INTERVAL = settings.tempUpdateInterval * 1000;
    if (dataLogger) {
        delete dataLogger;
        dataLogger = new DataLogger("/temperature_log.json", settings.loggingInterval);
        if (spiffsInitialized) {
            dataLogger->begin();
        }
    }
}

void handleReset() {
    Serial.println("Reset triggered - deleting WiFi configuration");
    digitalWrite(RED_LED, LOW);  // Turn off red LED
    
    if (wifiManager->deleteCredentials()) {
        // Start blinking blue LED
        Serial.println("WiFi configuration deleted. Restarting device...");
        // Blink blue LED a few times before restart
        for(int i = 0; i < 6; i++) {
            digitalWrite(BLUE_LED, HIGH);
            delay(250);
            digitalWrite(BLUE_LED, LOW);
            delay(250);
        }
        ESP.restart();
    }
}

void handleWiFiCredentials(const char* ssid, const char* password) {
    if (wifiManager->saveCredentials(ssid, password)) {
        delay(1000);
        ESP.restart();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Starting IoT Temperature Monitor...");

    // Initialize LED pins
    pinMode(RED_LED, OUTPUT);
    pinMode(BLUE_LED, OUTPUT);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BLUE_LED, LOW);

    // Initialize SPIFFS with retry logic
    int retryCount = 0;
    while (!spiffsInitialized && retryCount < 3) {
        if (initializeSPIFFS()) {
            break;
        }
        retryCount++;
        delay(1000);
    }

    if (!spiffsInitialized) {
        // If SPIFFS fails, indicate error with RED LED
        digitalWrite(RED_LED, HIGH);
        Serial.println("Critical: Failed to initialize SPIFFS after retries");
    }

    loadSettings();

    // Initialize managers
    sensorManager = new SensorManager(TEMPERATURE_SENSOR);
    wifiManager = new WifiManager();
    webServerManager = new WebServerManager();
    
    // Set up callbacks immediately after creating webServerManager
    webServerManager->setSystemSettingsCallback(handleSystemSettings);
    
    // Then continue with initialization
    resetManager = new ResetManager(RESET_BUTTON);
    
    if (spiffsInitialized) {
        dataLogger = new DataLogger("/temperature_log.json", settings.loggingInterval);
    }

    // Initialize components
    if (!sensorManager->begin()) {
        Serial.println("Failed to initialize temperature sensor!");
    }

    if (!wifiManager->begin()) {
        Serial.println("Failed to initialize WiFi!");
    }

    if (!webServerManager->begin()) {
        Serial.println("Failed to initialize web server!");
    }

    if (!resetManager->begin()) {
        Serial.println("Failed to initialize reset manager!");
    }

    if (spiffsInitialized && dataLogger && !dataLogger->begin()) {
        Serial.println("Failed to initialize data logger!");
    }

    // Set up callbacks
    webServerManager->setWiFiCredentialsCallback(handleWiFiCredentials);
    webServerManager->setSystemResetCallback(handleReset);
    resetManager->setResetCallback(handleReset);

    // Set AP mode state based on WiFi connection
    webServerManager->setAPMode(!wifiManager->isConnected());

    // Print network information
    if (wifiManager->isConnected()) {
        Serial.print("Connected to WiFi. IP: ");
        Serial.println(WiFi.localIP());
        
        // Configure NTP
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        
        // Wait for time to be set
        time_t now = time(nullptr);
        while (now < 24 * 3600) {
            Serial.println("Waiting for NTP time sync...");
            delay(500);
            now = time(nullptr);
        }
        
        Serial.println("Time synchronized with NTP server");
    } else {
        Serial.print("AP Mode. IP: ");
        Serial.println(WiFi.softAPIP());
    }
}

void loop() {
    resetManager->check();

    if (millis() - lastTempUpdate >= TEMP_UPDATE_INTERVAL) {
        lastTempUpdate = millis();
        
        if (sensorManager->isSensorWorking()) {
            float temperature = sensorManager->getTemperature();
            
            // Get current timestamp
            time_t now;
            time(&now);
            struct tm timeinfo;
            localtime_r(&now, &timeinfo);
            char timeStr[20];
            strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
            
            // Log temperature with timestamp
            if (wifiManager->isConnected()) {
                Serial.printf("WiFi Mode - Time: %s, Temperature: %.1f°C (logged)\n", timeStr, temperature);
                
                // Only try to log if SPIFFS is initialized and we're in WiFi mode
                if (spiffsInitialized && dataLogger) {
                    // Check if it's time to log the temperature
                    if (dataLogger->shouldLog()) {
                        time_t now = time(nullptr);
                        // Only log if we have valid NTP time (timestamp > Jan 1, 2024)
                        if (now > 1704067200) {  // Unix timestamp for Jan 1, 2024
                            if (!dataLogger->logTemperature(temperature)) {
                                // Try to reinitialize SPIFFS if logging fails
                                if (initializeSPIFFS()) {
                                    dataLogger->logTemperature(temperature);
                                }
                            }
                        }
                    }
                }
            } else {
                Serial.printf("AP Mode - Temperature: %.1f°C (no logs)\n", temperature);
            }
            
            // Broadcast to WebSocket clients without serial output
            webServerManager->broadcastTemperature(temperature);
        } else {
            Serial.println("Error reading temperature sensor!");
        }
    }

    delay(10);
}
