#include <Arduino.h>
#include "SensorManager.h"
#include "WifiManager.h"
#include "WebServerManager.h"
#include "ResetManager.h"
#include <time.h>

// NTP Server settings
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;      // GMT offset (in seconds)
const int daylightOffset_sec = 3600;  // Daylight offset (in seconds)

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

// Timer for temperature updates
unsigned long lastTempUpdate = 0;
const unsigned long TEMP_UPDATE_INTERVAL = 5000; // 5 seconds

// LED blinking variables
bool isBlinking = false;
unsigned long lastBlinkTime = 0;
const unsigned long BLINK_INTERVAL = 500; // 500ms blink interval

void handleReset() {
    Serial.println("Reset triggered - deleting WiFi configuration");
    digitalWrite(RED_LED, LOW);  // Turn off red LED
    
    if (wifiManager->deleteCredentials()) {
        // Start blinking blue LED
        isBlinking = true;
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

    // Initialize managers
    sensorManager = new SensorManager(TEMPERATURE_SENSOR);
    wifiManager = new WifiManager();
    webServerManager = new WebServerManager();
    resetManager = new ResetManager(RESET_BUTTON);

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

    // Set up callbacks
    webServerManager->setWiFiCredentialsCallback(handleWiFiCredentials);
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
            Serial.printf("[%s] Temperature: %.1f°C\n", timeStr, temperature);
            
            // Broadcast to WebSocket clients
            webServerManager->broadcastTemperature(temperature);
        } else {
            Serial.println("Error reading temperature sensor!");
        }
    }

    delay(10);
}
