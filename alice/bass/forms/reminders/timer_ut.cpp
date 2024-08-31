#include "timer.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS::NReminders;

namespace {
Y_UNIT_TEST_SUITE(TimerUnitTest) {
    Y_UNIT_TEST(AreJsonDirectivesEqualByName) {
        const auto value = NSc::TValue::FromJson(TStringBuf(R"(
        [
            {
                "name": "player_pause",
                "type": "client_action",
                "payload": {
                    "smooth": true
                }
            },
            {
                "name": "go_home",
                "type": "client_action"
            },
            {
                "name": "screen_off",
                "type": "client_action"
            }
        ]
        )"));
        UNIT_ASSERT(NImpl::AreJsonDirectivesEqualByName(value, value));
    }

    Y_UNIT_TEST(AreJsonDirectivesEqualByName_Backward) {
        const auto lhs = NSc::TValue::FromJson(TStringBuf(R"(
        [
            {
                "name": "player_pause",
                "type": "client_action",
                "payload": {
                    "smooth": true
                }
            },
            {
                "name": "go_home",
                "type": "client_action"
            },
            {
                "name": "screen_off",
                "type": "client_action"
            }
        ]
        )"));
        const auto rhs = NSc::TValue::FromJson(TStringBuf(R"(
        [
            {
                "name": "player_pause",
                "payload": {
                    "smooth": true
                }
            },
            {
                "name": "go_home"
            },
            {
                "name": "screen_off"
            }
        ]
        )"));

        UNIT_ASSERT_C(NImpl::AreJsonDirectivesEqualByName(lhs, rhs),
                      "Lhs(" << lhs << ") has to be equal to Rhs(" << rhs << ") by name");
        UNIT_ASSERT_C(NImpl::AreJsonDirectivesEqualByName(rhs, lhs),
                      "Lhs(" << rhs << ") has to be equal to Rhs(" << lhs << ") by name");
    }

    Y_UNIT_TEST(AreJsonDirectivesEqualByName_DifferentDirectives) {
        const auto lhs = NSc::TValue::FromJson(TStringBuf(R"(
        [
            {
                "name": "player_pause",
                "type": "client_action",
                "payload": {
                    "smooth": true
                }
            },
            {
                "name": "go_home",
                "type": "client_action"
            },
            {
                "name": "screen_off",
                "type": "client_action"
            }
        ]
        )"));
        const auto rhs = NSc::TValue::FromJson(TStringBuf(R"(
        [
            {
                "name": "player_pause",
                "payload": {
                    "smooth": true
                }
            }
        ]
        )"));

        UNIT_ASSERT_C(!NImpl::AreJsonDirectivesEqualByName(lhs, rhs),
                      "Lhs(" << lhs << ") should not be equal to Rhs(" << rhs << ") by name");
        UNIT_ASSERT_C(!NImpl::AreJsonDirectivesEqualByName(rhs, lhs),
                      "Lhs(" << rhs << ") should not be equal to Rhs(" << lhs << ") by name");
    }

    Y_UNIT_TEST(AreJsonDirectivesEqualByName_WrongTypes) {
        const auto lhs = NSc::TValue::FromJson(TStringBuf("[]"));
        const auto rhs = NSc::TValue::FromJson(TStringBuf("{}"));

        UNIT_ASSERT_C(!NImpl::AreJsonDirectivesEqualByName(lhs, rhs),
                      "Lhs(" << lhs << ") should not be equal to Rhs(" << rhs << ") by name");
        UNIT_ASSERT_C(!NImpl::AreJsonDirectivesEqualByName(rhs, lhs),
                      "Lhs(" << rhs << ") should not be equal to Rhs(" << lhs << ") by name");
    }
}
} // namespace
