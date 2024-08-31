#include "aggregate_labels_builder.h"

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

Y_UNIT_TEST_SUITE(TAggregateLabelsBuilderTest) {
    Y_UNIT_TEST(TwoAdditionalDimensions) {

        TAggregateLabelsBuilder builder {"any_", {
            {"uid", "code"},
            {"code"},
            {"uid"},
        }};
        const auto vectorOfLabels = builder.Build({
            {"name", "test_sensor"},
            {"uid", "qwerty"},
            {"code", "42"}
        });
        UNIT_ASSERT_VALUES_EQUAL(3, vectorOfLabels.size());

        {
            const auto labels = vectorOfLabels[0];
            UNIT_ASSERT_VALUES_EQUAL(3, labels.Size());
            UNIT_ASSERT_VALUES_EQUAL("test_sensor", labels.Get("name").value()->Value());
            UNIT_ASSERT_VALUES_EQUAL("any_uid", labels.Get("uid").value()->Value());
            UNIT_ASSERT_VALUES_EQUAL("any_code", labels.Get("code").value()->Value());
        }
        {
            const auto labels = vectorOfLabels[1];
            UNIT_ASSERT_VALUES_EQUAL(3, labels.Size());
            UNIT_ASSERT_VALUES_EQUAL("test_sensor", labels.Get("name").value()->Value());
            UNIT_ASSERT_VALUES_EQUAL("qwerty", labels.Get("uid").value()->Value());
            UNIT_ASSERT_VALUES_EQUAL("any_code", labels.Get("code").value()->Value());
        }
        {
            const auto labels = vectorOfLabels[2];
            UNIT_ASSERT_VALUES_EQUAL(3, labels.Size());
            UNIT_ASSERT_VALUES_EQUAL("test_sensor", labels.Get("name").value()->Value());
            UNIT_ASSERT_VALUES_EQUAL("any_uid", labels.Get("uid").value()->Value());
            UNIT_ASSERT_VALUES_EQUAL("42", labels.Get("code").value()->Value());
        }
    }
}

} // namespace
