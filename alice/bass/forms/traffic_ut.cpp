#include "traffic.h"

#include <alice/bass/ut/helpers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NBASS::NTraffic::NImpl;

namespace {

constexpr auto TIMESTAMP = 1600000000;

constexpr TStringBuf TRAFFIC_RESPONSE = R"({
    "levels": {
        "predictions": [
            {
                "region": 213,
                "predictions": [
                    {
                        "timestamp": 1585036800,
                        "level": 4
                    },
                    {
                        "timestamp": 1585040400,
                        "level": 4
                    }
                ]
            }
        ]
    }
})";

constexpr TStringBuf OLD_TRAFFIC_RESPONSE = R"({
    "213": {
        "jams": {
            "11": {
                "from": "4"
            },
            "12": {
                "from": "4"
            }
        },
        "prediction_model": "new",
        "timestamp": 1600000000
    }
})";

Y_UNIT_TEST_SUITE(TrafficImpl) {
    Y_UNIT_TEST(TransformTrafficResponseToOldApiVersion) {
        NTestingHelpers::TTestGlobalContext context{};
        const auto actual = TransformTrafficResponseToOldApiVersion(NSc::TValue::FromJson(TRAFFIC_RESPONSE), TIMESTAMP,
                                                                    context.GeobaseLookup());
        const auto expected = NSc::TValue::FromJson(OLD_TRAFFIC_RESPONSE);
        UNIT_ASSERT_VALUES_EQUAL(actual, expected);
    }

    Y_UNIT_TEST(TransformTrafficResponseToOldApiVersionInvalidFormat) {
        NTestingHelpers::TTestGlobalContext context{};
        const auto actual = TransformTrafficResponseToOldApiVersion(NSc::TValue::FromJson("{}"), TIMESTAMP,
                                                                    context.GeobaseLookup());
        UNIT_ASSERT(actual.IsNull());
    }
}

} // namespace
