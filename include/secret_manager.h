#ifndef SECRET_MANAGER_H
#define SECRET_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "debug.h"

class SecretManager {
private:
    Preferences prefs;
    bool ready;

    String getSecret(const char* key) {
        if (!ready) return "";
        if (!prefs.isKey(key)) return "";
        return prefs.getString(key, "");
    }

    void applySecret(JsonVariant target, const char* key) {
        if (!ready || !target.isNull()) return;

        String secret = getSecret(key);
        if (secret.length() > 0) {
            target.set(secret);
        }
    }

    void storeSecret(JsonVariant source, const char* key) {
        if (!ready || source.isNull()) return;

        String secret = source.as<String>();
        if (secret.length() == 0) {
            prefs.remove(key);
        } else {
            prefs.putString(key, secret);
        }
    }

public:
    SecretManager() : ready(false) {}

    bool begin() {
        ready = prefs.begin("cisterna", false);
        if (!ready) {
            DBG_ERRORLN("[Secrets] Failed to open NVS namespace");
        }
        return ready;
    }

    void apply(JsonDocument& doc) {
        applySecret(doc["wifi_pass"], "wifi_pass");
        applySecret(doc["grafana"]["token"], "grafana_token");
        applySecret(doc["admin"]["password"], "admin_pass");
        applySecret(doc["telegram"]["bot_token"], "telegram_token");
    }

    void absorb(JsonDocument& doc) {
        storeSecret(doc["wifi_pass"], "wifi_pass");
        storeSecret(doc["grafana"]["token"], "grafana_token");
        storeSecret(doc["admin"]["password"], "admin_pass");
        storeSecret(doc["telegram"]["bot_token"], "telegram_token");

        doc.remove("wifi_pass");

        JsonObject grafana = doc["grafana"];
        if (!grafana.isNull()) {
            grafana.remove("token");
        }

        JsonObject admin = doc["admin"];
        if (!admin.isNull()) {
            admin.remove("password");
        }

        JsonObject telegram = doc["telegram"];
        if (!telegram.isNull()) {
            telegram.remove("bot_token");
        }
    }

    void redact(JsonDocument& doc) {
        doc.remove("wifi_pass");
        doc["wifi_pass_configured"] = getSecret("wifi_pass").length() > 0;

        JsonObject grafana = doc["grafana"];
        if (!grafana.isNull()) {
            grafana.remove("token");
            grafana["token_configured"] = getSecret("grafana_token").length() > 0;
        }

        JsonObject admin = doc["admin"];
        if (!admin.isNull()) {
            admin.remove("password");
            admin["password_configured"] = getSecret("admin_pass").length() > 0;
        }

        JsonObject telegram = doc["telegram"];
        if (!telegram.isNull()) {
            telegram.remove("bot_token");
            telegram["bot_token_configured"] = getSecret("telegram_token").length() > 0;
        }
    }

    String getAdminPassword() {
        return getSecret("admin_pass");
    }

    bool hasAdminPassword() {
        return getSecret("admin_pass").length() >= 8;
    }
};

#endif // SECRET_MANAGER_H
