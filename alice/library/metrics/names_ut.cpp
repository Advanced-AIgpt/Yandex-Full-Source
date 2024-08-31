#include "names.h"

#include <library/cpp/testing/unittest/registar.h>

namespace {

Y_UNIT_TEST_SUITE(SignalNames) {
    Y_UNIT_TEST(ClientType) {
        using namespace NAlice::NSignal;

        // aliced as prefix
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("aliced"), "smart_speaker");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("alicederevnya"), "smart_speaker");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("aliced.aliced"), "smart_speaker");

        // quasar as prefix
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("ru.yandex.quasar"), "smart_speaker");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("ru.yandex.quasar.proka4ka"), "smart_speaker");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("ru.yandex.quasar.mini"), "smart_speaker");

        // common fails
        UNIT_ASSERT_VALUES_EQUAL(GetClientType(""), "other");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("prikol"), "other");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("aliceed"), "other");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("ru.yandex."), "other");
        UNIT_ASSERT_VALUES_EQUAL(GetClientType("ru.yandex.quasa"), "other");
    }
}

} // namespace
