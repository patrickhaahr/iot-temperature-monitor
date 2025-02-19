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
        return false;
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
        attempts--;
    }

    return WiFi.status() == WL_CONNECTED;
}

void WifiManager::startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPassword);
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
