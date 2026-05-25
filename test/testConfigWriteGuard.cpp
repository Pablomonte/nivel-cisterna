// ConfigWriteGuard replicado desde src/main.cpp.
// El guard RAII protege escrituras de configuracion contra solapamientos:
// el primer Guard que se construye con el flag en false adquiere; el resto
// observa busy y NO toca el flag. Solo el guard que adquirio libera al
// destruirse. Esto mantiene la invariante "una sola escritura activa".

#include <unity.h>

namespace {

volatile bool configWriteBusy = false;

struct ConfigWriteGuard {
    bool acquired;
    ConfigWriteGuard() : acquired(false) {
        if (!configWriteBusy) {
            configWriteBusy = true;
            acquired = true;
        }
    }
    ~ConfigWriteGuard() {
        if (acquired) configWriteBusy = false;
    }
};

void resetFlag() { configWriteBusy = false; }

}  // namespace

void testConfigWriteGuard_AcquireWhenFreeSetsFlag() {
    resetFlag();
    {
        ConfigWriteGuard g;
        TEST_ASSERT_TRUE(g.acquired);
        TEST_ASSERT_TRUE(configWriteBusy);
    }
    TEST_ASSERT_FALSE(configWriteBusy);  // RAII libero al salir del scope
}

void testConfigWriteGuard_AcquireWhenBusyDoesNotAcquire() {
    resetFlag();
    ConfigWriteGuard owner;
    TEST_ASSERT_TRUE(owner.acquired);

    {
        ConfigWriteGuard intruder;
        TEST_ASSERT_FALSE(intruder.acquired);
        TEST_ASSERT_TRUE(configWriteBusy);  // sigue prendido por el owner
    }
    // El destructor del intruder NO debe haber liberado el flag.
    TEST_ASSERT_TRUE(configWriteBusy);
}

void testConfigWriteGuard_NestedScopesOnlyOwnerReleases() {
    resetFlag();
    {
        ConfigWriteGuard outer;
        TEST_ASSERT_TRUE(outer.acquired);
        {
            ConfigWriteGuard inner;
            TEST_ASSERT_FALSE(inner.acquired);
        }
        // inner salio de scope: flag debe seguir prendido por outer.
        TEST_ASSERT_TRUE(configWriteBusy);
    }
    TEST_ASSERT_FALSE(configWriteBusy);  // outer libero al salir
}

void testConfigWriteGuard_SequentialAcquireRelease() {
    resetFlag();
    {
        ConfigWriteGuard first;
        TEST_ASSERT_TRUE(first.acquired);
    }
    TEST_ASSERT_FALSE(configWriteBusy);

    {
        ConfigWriteGuard second;
        TEST_ASSERT_TRUE(second.acquired);
    }
    TEST_ASSERT_FALSE(configWriteBusy);
}
