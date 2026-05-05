// Reglas del handler POST /api/admin/password (src/main.cpp::handleApiAdminPassword)
// replicadas con std::string para validar invariantes sin Arduino/WebServer.
//
// Reglas:
//   - new debe tener largo en [8, 64]
//   - si ya hay password configurada, current debe coincidir
//   - new no puede ser igual a la actual
//   - si NO hay password configurada (default MAC), permitir el primer set sin
//     verificar `current`

#include <unity.h>
#include <string>

namespace {

enum class AdminPwdResult {
    Ok,
    NewTooShort,
    NewTooLong,
    CurrentMismatch,
    SameAsCurrent
};

struct AdminPwdState {
    bool configured = false;          // hasAdminPassword() en el firmware
    std::string stored;               // valor actual en NVS (si configured)
};

AdminPwdResult tryChangeAdminPassword(AdminPwdState& state,
                                      const std::string& current,
                                      const std::string& next) {
    if (next.size() < 8) return AdminPwdResult::NewTooShort;
    if (next.size() > 64) return AdminPwdResult::NewTooLong;

    if (state.configured) {
        if (current != state.stored) {
            return AdminPwdResult::CurrentMismatch;
        }
    }

    if (next == state.stored) {
        return AdminPwdResult::SameAsCurrent;
    }

    state.stored = next;
    state.configured = true;
    return AdminPwdResult::Ok;
}

}  // namespace

void testAdminPassword_RejectsShort() {
    AdminPwdState state{ true, "currentpass" };
    auto r = tryChangeAdminPassword(state, "currentpass", "1234567");
    TEST_ASSERT_EQUAL_INT((int)AdminPwdResult::NewTooShort, (int)r);
    TEST_ASSERT_EQUAL_STRING("currentpass", state.stored.c_str());
}

void testAdminPassword_RejectsTooLong() {
    AdminPwdState state{ true, "currentpass" };
    auto r = tryChangeAdminPassword(state, "currentpass", std::string(65, 'a'));
    TEST_ASSERT_EQUAL_INT((int)AdminPwdResult::NewTooLong, (int)r);
}

void testAdminPassword_RejectsSameAsCurrent() {
    AdminPwdState state{ true, "samepass1" };
    auto r = tryChangeAdminPassword(state, "samepass1", "samepass1");
    TEST_ASSERT_EQUAL_INT((int)AdminPwdResult::SameAsCurrent, (int)r);
}

void testAdminPassword_RequiresCurrentWhenConfigured() {
    AdminPwdState state{ true, "realcurrent" };
    auto r = tryChangeAdminPassword(state, "wrongone", "newpass123");
    TEST_ASSERT_EQUAL_INT((int)AdminPwdResult::CurrentMismatch, (int)r);
    TEST_ASSERT_EQUAL_STRING("realcurrent", state.stored.c_str());
}

void testAdminPassword_AllowsFirstSetWhenUnconfigured() {
    AdminPwdState state{ false, "" };
    auto r = tryChangeAdminPassword(state, "", "firstpass1");
    TEST_ASSERT_EQUAL_INT((int)AdminPwdResult::Ok, (int)r);
    TEST_ASSERT_TRUE(state.configured);
    TEST_ASSERT_EQUAL_STRING("firstpass1", state.stored.c_str());
}
