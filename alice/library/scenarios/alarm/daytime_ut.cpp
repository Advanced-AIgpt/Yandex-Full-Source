#include "date_time.h"

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NAlice::NScenarios::NAlarm;
using namespace NDatetime;

using TComponent = TDayTime::TComponent;

namespace {
const NSc::TValue SAMPLE = NSc::TValue::FromJson(R"({
    "hours": 13,
    "minutes": 2,
    "seconds": 15
})");

const NSc::TValue SAMPLE_AM = NSc::TValue::FromJson(R"({
    "hours": 1,
    "minutes": 59,
    "seconds": 31,
    "period": "am"
})");

const NSc::TValue SAMPLE_PM = NSc::TValue::FromJson(R"({
    "hours": 11,
    "minutes": 0,
    "seconds": 59,
    "period": "pm"
})");

const NSc::TValue SAMPLE_MIDNIGHT = NSc::TValue::FromJson(R"({
    "hours": 12,
    "period": "am"
})");

const NSc::TValue SAMPLE_PM_WITH_21_HOUR = NSc::TValue::FromJson(R"({
    "hours": 21,
    "period": "pm"
})");

const NSc::TValue SAMPLE_AFTER_MIDNIGHT = NSc::TValue::FromJson(R"({
    "hours": 12,
    "minutes": 1,
    "seconds": 2,
    "period": "am"
})");

const NSc::TValue SAMPLE_NOON = NSc::TValue::FromJson(R"({
    "hours": 12,
    "period": "pm"
})");

const NSc::TValue SAMPLE_AFTER_NOON = NSc::TValue::FromJson(R"({
    "hours": 12,
    "minutes": 2,
    "seconds": 3,
    "period": "pm"
})");

const NSc::TValue SAMPLE_HOURS_RELATIVE = NSc::TValue::FromJson(R"({
    "hours": 1,
    "hours_relative": true,
})");

const NSc::TValue SAMPLE_ABS_MINUTES_REL_SECONDS = NSc::TValue::FromJson(R"({
    "minutes": 10,
    "minutes_relative": false,
    "seconds": 55,
    "seconds_relative": true
})");

const NSc::TValue SAMPLE_REL_HOURS_REL_SECONDS = NSc::TValue::FromJson(R"({
    "hours": 2,
    "hours_relative": true,
    "minutes": 30,
    "minutes_relative": true
})");

const TCivilSecond NOW{2018, 2, 28, 10, 0, 0};

Y_UNIT_TEST_SUITE(DayTimeUnitTest) {
    Y_UNIT_TEST(Sample) {
        const auto dayTime = TDayTime::FromValue(SAMPLE);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime,
            TDayTime(13 /* hours */, 2 /* minutes */, 15 /* seconds */, TDayTime::EPeriod::Unspecified));
    }

    Y_UNIT_TEST(SampleUnspecified) {
        {
            const TDayTime dayTime(TComponent(8, false) /* hours */, Nothing() /* minutes */, Nothing() /* seconds */,
                                   TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(dayTime.Apply(NOW), TCivilSecond(2018, 2, 28, 8, 0, 0));
        }

        {
            const TDayTime dayTime(TComponent(11, false) /* relative */, Nothing() /* minutes */,
                                   Nothing() /* seconds */, TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(dayTime.Apply(NOW), TCivilSecond(2018, 2, 28, 11, 0, 0));
        }

        {
            const TCivilSecond now{2018, 2, 28, 14, 0, 0};
            const TDayTime dayTime(TComponent(12, false) /* relative */, Nothing() /* minutes */,
                                   Nothing() /* seconds */, TDayTime::EPeriod::Unspecified);
            UNIT_ASSERT_VALUES_EQUAL(dayTime.Apply(now), TCivilSecond(2018, 2, 28, 12, 0, 0));
        }
    }

    Y_UNIT_TEST(SampleAM) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_AM);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime,
            TDayTime(1 /* hours */, 59 /* minutes */, 31 /* seconds */, TDayTime::EPeriod::AM));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 1, 59, 31));
    }

    Y_UNIT_TEST(SamplePM) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_PM);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime,
            TDayTime(11 /* hours */, 0 /* minutes */, 59 /* seconds */, TDayTime::EPeriod::PM));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 23, 0, 59));
    }

    Y_UNIT_TEST(Sample_PM_With_21_Hour) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_PM_WITH_21_HOUR);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime,
            TDayTime(TComponent(21, false)/* hours */, Nothing() /* minutes */, Nothing() /* seconds */, TDayTime::EPeriod::Unspecified));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 21, 0, 0));
    }

    Y_UNIT_TEST(SampleMidnight) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_MIDNIGHT);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime,
            TDayTime(TComponent(12, false) /* hours */, Nothing() /* minutes */,
                Nothing() /* seconds */, TDayTime::EPeriod::AM));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 0, 0, 0));
    }

    Y_UNIT_TEST(SampleAfterMidnight) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_AFTER_MIDNIGHT);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime, TDayTime(12 /* hours */, 1 /* minutes */, 2 /* seconds */, TDayTime::EPeriod::AM));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 0, 1, 2));
    }

    Y_UNIT_TEST(SampleNoon) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_NOON);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime, TDayTime(TComponent(12, false) /* hours */, Nothing() /* minutes */,
            Nothing() /* seconds */, TDayTime::EPeriod::PM));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 12, 0, 0));
    }

    Y_UNIT_TEST(SampleAfterNoon) {
        const auto dayTime = TDayTime::FromValue(SAMPLE_AFTER_NOON);

        UNIT_ASSERT(dayTime);
        UNIT_ASSERT_VALUES_EQUAL(*dayTime, TDayTime(12 /* hours */, 2 /* minutes */, 3 /* seconds */, TDayTime::EPeriod::PM));
        UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 12, 2, 3));
    }

    Y_UNIT_TEST(SampleRelative) {
        {
            const auto dayTime = TDayTime::FromValue(SAMPLE_HOURS_RELATIVE);
            UNIT_ASSERT(dayTime);
            UNIT_ASSERT_VALUES_EQUAL(*dayTime, TDayTime(TComponent(1, true) /* hours */, Nothing() /* minutes */,
                Nothing() /* seconds */, TDayTime::EPeriod::Unspecified));
            UNIT_ASSERT(dayTime->IsRelative());
            UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(NOW), TCivilSecond(2018, 2, 28, 11, 0, 0));
        }

        {
            const auto dayTime = TDayTime::FromValue(SAMPLE_ABS_MINUTES_REL_SECONDS);
            UNIT_ASSERT(dayTime);
            UNIT_ASSERT_VALUES_EQUAL(*dayTime,
                TDayTime(Nothing() /* hours */, TComponent(10, false) /* minutes */,
                    TComponent(55, true) /* seconds */, TDayTime::EPeriod::Unspecified));
            UNIT_ASSERT(!dayTime->IsRelative());

            UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(TCivilSecond{2018, 2, 28, 23, 55, 1}),
                                     TCivilSecond(2018, 2, 28, 0, 10, 56));
            UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(TCivilSecond{2018, 2, 28, 23, 55, 9}),
                                     TCivilSecond(2018, 2, 28, 0, 11, 4));
        }

        {
            const auto dayTime = TDayTime::FromValue(SAMPLE_REL_HOURS_REL_SECONDS);
            UNIT_ASSERT(dayTime);
            UNIT_ASSERT_VALUES_EQUAL(*dayTime,
                TDayTime(TComponent(2, true) /* hours */, TComponent(30, true) /* minutes */,
                    Nothing(), TDayTime::EPeriod::Unspecified));
            UNIT_ASSERT(dayTime->IsRelative());

            UNIT_ASSERT_VALUES_EQUAL(dayTime->Apply(TCivilSecond{2018, 2, 28, 23, 55, 9}),
                                     TCivilSecond(2018, 3, 1, 2, 25, 9));
        }
    }

    Y_UNIT_TEST(Invalid) {
        {
            const NSc::TValue sample = NSc::TValue::FromJson(R"({"hours": 25})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample = NSc::TValue::FromJson(R"({"hours": 13, "minutes": -100})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample = NSc::TValue::FromJson(R"({"hours": 10, "minutes": 10, "seconds": null})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample = NSc::TValue::FromJson(R"({"hours": 1.0, "minutes": 2.0, "seconds": 3.0})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample =
                NSc::TValue::FromJson(R"({"hours": 1, "minutes": 2, "seconds": 3, "period": "вечер"})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample =
                NSc::TValue::FromJson(R"({"hours": 0, "minutes": 0, "seconds": 0, "period": "am"})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample =
                NSc::TValue::FromJson(R"({"hours": 0, "minutes": 0, "seconds": 0, "period": "pm"})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
        {
            const NSc::TValue sample =
                NSc::TValue::FromJson(R"({"hours": 10, "hours_relative": true, "period": "pm"})");
            UNIT_ASSERT(!TDayTime::FromValue(sample));
        }
    }

    Y_UNIT_TEST(FromCivilTime) {
        static const TVector<std::pair<TCivilSecond, TStringBuf>> items = {
            { TCivilSecond{2018, 3, 22, 0, 0, 0},   R"({"hours":12,"minutes":0,"period":"am","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 0, 1, 0},   R"({"hours":12,"minutes":1,"period":"am","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 1, 0, 0},   R"({"hours":1,"minutes":0,"period":"am","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 11, 0, 0},  R"({"hours":11,"minutes":0,"period":"am","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 11, 59, 0}, R"({"hours":11,"minutes":59,"period":"am","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 12, 0, 0},  R"({"hours":12,"minutes":0,"period":"pm","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 12, 1, 0},  R"({"hours":12,"minutes":1,"period":"pm","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 13, 0, 0},  R"({"hours":1,"minutes":0,"period":"pm","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 23, 0, 0},  R"({"hours":11,"minutes":0,"period":"pm","seconds":0})" },
            { TCivilSecond{2018, 3, 22, 23, 59, 0}, R"({"hours":11,"minutes":59,"period":"pm","seconds":0})" },
        };

        for (const auto& item : items) {
            UNIT_ASSERT_VALUES_EQUAL(TDayTime(item.first).ToValue().ToJson(), item.second);
        }
    }
}
} // namespace
