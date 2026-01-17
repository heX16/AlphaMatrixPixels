#pragma once

// WiFi + OTA helper.
// - On ESP8266/ESP32: connects to WiFi (STA) and runs ArduinoOTA.
// - On other boards (e.g. Arduino Nano): compiles to no-op stubs.
//
// Configuration lives in `led_config.hpp`:
// - AMP_WIFI_SSID, AMP_WIFI_PASSWORD
// - AMP_OTA_HOSTNAME, AMP_OTA_PASSWORD
// - AMP_ENABLE_WIFI_OTA (0/1)

#include <Arduino.h>

#if defined(ESP8266) || defined(ESP32)
  #if defined(ESP8266)
    #include <ESP8266WiFi.h>
  #elif defined(ESP32)
    #include <WiFi.h>
  #endif
  #include <ArduinoOTA.h>
#endif

inline void setupOTA() {
#if (defined(ESP8266) || defined(ESP32)) && defined(AMP_ENABLE_WIFI_OTA) && (AMP_ENABLE_WIFI_OTA != 0)
  // Skip if SSID is not provided.
  if (sizeof(AMP_WIFI_SSID) <= 1) {
    return;
  }

  WiFi.mode(WIFI_STA);

  // Device hostname (DHCP name shown in the router).
  // Reuse AMP_OTA_HOSTNAME to keep configuration in one place.
  if (sizeof(AMP_OTA_HOSTNAME) > 1) {
    #if defined(ESP8266)
      WiFi.hostname(AMP_OTA_HOSTNAME);
    #elif defined(ESP32)
      WiFi.setHostname(AMP_OTA_HOSTNAME);
    #endif
  }

  WiFi.setAutoReconnect(true);
  WiFi.begin(AMP_WIFI_SSID, AMP_WIFI_PASSWORD);

  // Configure OTA callbacks once. We'll call ArduinoOTA.begin() only after WiFi connects.
  ArduinoOTA.setHostname(AMP_OTA_HOSTNAME);

  if (sizeof(AMP_OTA_PASSWORD) > 1) {
    ArduinoOTA.setPassword(AMP_OTA_PASSWORD);
  }

  ArduinoOTA.onStart([]() {
    // Keep handlers lightweight to avoid timing issues.
  });
  ArduinoOTA.onEnd([]() {});
  ArduinoOTA.onProgress([](unsigned int /*progress*/, unsigned int /*total*/) {});
  ArduinoOTA.onError([](ota_error_t /*error*/) {});
#endif
}

inline void handleOTA() {
#if (defined(ESP8266) || defined(ESP32)) && defined(AMP_ENABLE_WIFI_OTA) && (AMP_ENABLE_WIFI_OTA != 0)
  static bool ota_started = false;

  if (sizeof(AMP_WIFI_SSID) <= 1) {
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!ota_started) {
      ArduinoOTA.begin();
      ota_started = true;
    }
    ArduinoOTA.handle();
    yield();
  }
#endif
}


