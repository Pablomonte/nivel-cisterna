// Logica de SecretManager::getSecret replicada con un MockPrefs.
// Caso clave (regresion protegida): si la key no existe en NVS, getSecret
// debe retornar "" sin invocar getString(). Antes del fix vivo en
// include/secret_manager.h:16, getString podia devolver basura interpretada.

#include <unity.h>
#include <set>
#include <map>
#include <string>

namespace {

struct MockPrefs {
    std::map<std::string, std::string> store;
    bool isKey(const std::string& key) const { return store.count(key) > 0; }
    std::string getString(const std::string& key, const std::string& fallback) const {
        auto it = store.find(key);
        return it == store.end() ? fallback : it->second;
    }
};

class SecretManagerSpy {
public:
    MockPrefs prefs;
    bool ready = false;
    int getStringCalls = 0;

    std::string getSecret(const char* key) {
        if (!ready) return "";
        if (!prefs.isKey(key)) return "";
        ++getStringCalls;
        return prefs.getString(key, "");
    }
};

}  // namespace

void testSecretFallback_KeyMissing() {
    SecretManagerSpy mgr;
    mgr.ready = true;
    mgr.prefs.store["admin_pass"] = "supersecret";

    TEST_ASSERT_EQUAL_STRING("", mgr.getSecret("wifi_pass").c_str());
    TEST_ASSERT_EQUAL_INT(0, mgr.getStringCalls);

    TEST_ASSERT_EQUAL_STRING("supersecret", mgr.getSecret("admin_pass").c_str());
    TEST_ASSERT_EQUAL_INT(1, mgr.getStringCalls);
}

void testSecretFallback_NotReady() {
    SecretManagerSpy mgr;
    mgr.ready = false;
    mgr.prefs.store["admin_pass"] = "supersecret";

    TEST_ASSERT_EQUAL_STRING("", mgr.getSecret("admin_pass").c_str());
    TEST_ASSERT_EQUAL_INT(0, mgr.getStringCalls);
}
