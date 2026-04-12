#ifndef TELEGRAM_NOTIFIER_H
#define TELEGRAM_NOTIFIER_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include "debug.h"
#include "level_sensor.h"
#include "pump_controller.h"

static const char TELEGRAM_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

class TelegramNotifier {
private:
    bool enabled;
    String deviceName;
    String chatId;
    String botToken;
    int lowLevelThreshold;
    int recoveryThreshold;
    int sensorFailureThreshold;
    unsigned long cooldownMs;

    bool lowLevelActive;
    bool lowLevelAlertSent;
    bool sensorErrorActive;
    bool sensorErrorAlertSent;
    bool pumpTimeoutActive;
    bool pumpTimeoutAlertSent;

    unsigned long lastLowAttemptMs;
    unsigned long lastLowRecoveryAttemptMs;
    unsigned long lastSensorAttemptMs;
    unsigned long lastSensorRecoveryAttemptMs;
    unsigned long lastPumpAttemptMs;
    unsigned long lastPumpRecoveryAttemptMs;

    bool hasValidTime() const {
        return time(nullptr) > 1700000000;
    }

    bool cooldownElapsed(unsigned long& lastAttemptMs) {
        unsigned long now = millis();
        if (now - lastAttemptMs < cooldownMs) {
            return false;
        }
        lastAttemptMs = now;
        return true;
    }

    String urlEncode(const String& input) const {
        static const char hex[] = "0123456789ABCDEF";
        String encoded;
        encoded.reserve(input.length() * 3);

        for (size_t i = 0; i < input.length(); i++) {
            unsigned char c = static_cast<unsigned char>(input[i]);
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.' || c == '~') {
                encoded += static_cast<char>(c);
            } else {
                encoded += '%';
                encoded += hex[(c >> 4) & 0x0F];
                encoded += hex[c & 0x0F];
            }
        }

        return encoded;
    }

    bool sendMessage(const String& text, unsigned long& lastAttemptMs) {
        if (!enabled || botToken.length() == 0 || chatId.length() == 0) return false;
        if (WiFi.status() != WL_CONNECTED) return false;
        if (!cooldownElapsed(lastAttemptMs)) return false;
        if (!hasValidTime()) {
            DBG_ERRORLN("[Telegram] Clock not synced yet, skipping send");
            return false;
        }

        WiFiClientSecure client;
        client.setCACert(TELEGRAM_ROOT_CA);

        HTTPClient http;
        String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
        if (!http.begin(client, url)) {
            DBG_ERRORLN("[Telegram] Failed to open HTTP session");
            return false;
        }

        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        http.addHeader("Content-Type", "application/x-www-form-urlencoded");

        String payload = "chat_id=" + urlEncode(chatId) +
                         "&disable_web_page_preview=true&text=" +
                         urlEncode(text);

        int httpCode = http.POST(payload);
        http.end();

        if (httpCode >= 200 && httpCode < 300) {
            DBG_INFOLN("[Telegram] Notification sent");
            return true;
        }

        DBG_ERROR("[Telegram] Error: %d\n", httpCode);
        return false;
    }

public:
    TelegramNotifier() : enabled(false), lowLevelThreshold(15), recoveryThreshold(30),
                         sensorFailureThreshold(3), cooldownMs(15UL * 60UL * 1000UL),
                         lowLevelActive(false), lowLevelAlertSent(false),
                         sensorErrorActive(false), sensorErrorAlertSent(false),
                         pumpTimeoutActive(false), pumpTimeoutAlertSent(false),
                         lastLowAttemptMs(0), lastLowRecoveryAttemptMs(0),
                         lastSensorAttemptMs(0), lastSensorRecoveryAttemptMs(0),
                         lastPumpAttemptMs(0), lastPumpRecoveryAttemptMs(0) {}

    void loadFromConfig(JsonObject cfg, const char* device) {
        enabled = cfg["enabled"] | false;
        deviceName = String(device);
        chatId = cfg["chat_id"] | "";
        botToken = cfg["bot_token"] | "";
        lowLevelThreshold = cfg["low_level_threshold_pct"] | 15;
        recoveryThreshold = cfg["recovery_threshold_pct"] | 30;
        sensorFailureThreshold = cfg["sensor_failure_threshold"] | 3;
        cooldownMs = (cfg["cooldown_sec"] | 900) * 1000UL;

        DBG_INFO("[Telegram] %s cooldown=%lus\n",
                 enabled ? "enabled" : "disabled", cooldownMs / 1000UL);
    }

    bool isConfigured() const {
        return enabled && chatId.length() > 0 && botToken.length() > 0;
    }

    void update(const LevelSensor& sensor, const PumpController& pump) {
        if (!isConfigured()) return;

        float level = sensor.getLevel();
        bool hasValidLevel = sensor.isOk() && level >= 0.0f;
        bool lowCondition = hasValidLevel && level <= lowLevelThreshold;

        if (lowCondition) {
            lowLevelActive = true;
            if (!lowLevelAlertSent) {
                String message = "[" + deviceName + "] Nivel critico: " +
                                 String(level, 1) + "% (" +
                                 String(sensor.getVolume(), 0) + " L)";
                if (sendMessage(message, lastLowAttemptMs)) {
                    lowLevelAlertSent = true;
                }
            }
        } else if (lowLevelActive && hasValidLevel && level >= recoveryThreshold) {
            if (lowLevelAlertSent) {
                String message = "[" + deviceName + "] Nivel recuperado: " +
                                 String(level, 1) + "%";
                sendMessage(message, lastLowRecoveryAttemptMs);
            }
            lowLevelActive = false;
            lowLevelAlertSent = false;
        }

        bool sensorErrorCondition = sensor.getConsecutiveFailures() >= sensorFailureThreshold;
        if (sensorErrorCondition) {
            sensorErrorActive = true;
            if (!sensorErrorAlertSent) {
                String message = "[" + deviceName + "] Error de sensor: " +
                                 String(sensor.getConsecutiveFailures()) +
                                 " lecturas fallidas consecutivas";
                if (sendMessage(message, lastSensorAttemptMs)) {
                    sensorErrorAlertSent = true;
                }
            }
        } else if (sensorErrorActive && sensor.isOk()) {
            if (sensorErrorAlertSent) {
                String message = "[" + deviceName + "] Sensor recuperado";
                sendMessage(message, lastSensorRecoveryAttemptMs);
            }
            sensorErrorActive = false;
            sensorErrorAlertSent = false;
        }

        bool pumpTimeoutCondition = pump.getState() == PUMP_TIMEOUT;
        if (pumpTimeoutCondition) {
            pumpTimeoutActive = true;
            if (!pumpTimeoutAlertSent) {
                String message = "[" + deviceName + "] Bomba en timeout de seguridad";
                if (sendMessage(message, lastPumpAttemptMs)) {
                    pumpTimeoutAlertSent = true;
                }
            }
        } else if (pumpTimeoutActive) {
            if (pumpTimeoutAlertSent) {
                String message = "[" + deviceName + "] Bomba recuperada";
                sendMessage(message, lastPumpRecoveryAttemptMs);
            }
            pumpTimeoutActive = false;
            pumpTimeoutAlertSent = false;
        }
    }
};

#endif // TELEGRAM_NOTIFIER_H
