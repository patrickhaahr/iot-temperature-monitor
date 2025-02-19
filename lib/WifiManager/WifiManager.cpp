#include "WiFiManager.h"
#include <SPIFFS.h>
#include <WiFi.h>
#include <ArduinoJson.h>

const char* WifiManager::CONFIG_FILE = "/wifi.json";

WifiManager::WifiManager(const char* apSSID, const char* apPassword)
    : apSSID(apSSID), apPassword(apPassword) {
}

bool WifiManager::begin() {
    if (!SPIFFS.begin(true)) {
        Serial.println("Failed to mount SPIFFS");
        return false;
    }
    
    // Debug: List all files in SPIFFS
    Serial.println("\nListing files in SPIFFS:");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file) {
        Serial.print("- ");
        Serial.println(file.name());
        file = root.openNextFile();
    }
    
    // Debug: Print wifi.json contents if it exists
    if (SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("\nwifi.json contents:");
        File wifiConfig = SPIFFS.open(CONFIG_FILE, "r");
        while(wifiConfig.available()) {
            Serial.write(wifiConfig.read());
        }
        wifiConfig.close();
        Serial.println("\n");
    } else {
        Serial.println("\nNo wifi.json file found");
    }
    
    String ssid, password;
    if (loadCredentials(ssid, password)) {
        return connect();
    }
    
    startAPMode();
    return true;
}

bool WifiManager::connect() {
    String ssid, password;
    if (!loadCredentials(ssid, password)) {
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    // Wait for connection for 30 seconds
    int attempts = 30;
    while (WiFi.status() != WL_CONNECTED && attempts > 0) {
        delay(1000);
        Serial.print(".");
        attempts--;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConnected to WiFi network: %s\n", ssid.c_str());
        Serial.printf("Temperature monitor available at: http://%s\n\n", WiFi.localIP().toString().c_str());
        return true;
    }

    return false;
}

void WifiManager::startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
    
    Serial.println("\nAP Mode Started");
    Serial.printf("Network Name: %s\n", apSSID);
    Serial.printf("Password: %s\n", apPassword);
    Serial.printf("Configuration page available at: http://%s\n\n", WiFi.softAPIP().toString().c_str());
}

bool WifiManager::saveCredentials(const char* ssid, const char* password) {
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    File file = SPIFFS.open(CONFIG_FILE, "w");
    if (!file) {
        return false;
    }

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool WifiManager::deleteCredentials() {
    return SPIFFS.remove(CONFIG_FILE);
}

bool WifiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WifiManager::loadCredentials(String& ssid, String& password) {
    if (!SPIFFS.exists(CONFIG_FILE)) {
        return false;
    }

    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        return false;
    }

    ssid = doc["ssid"].as<String>();
    password = doc["password"].as<String>();
    
    return true;
}
