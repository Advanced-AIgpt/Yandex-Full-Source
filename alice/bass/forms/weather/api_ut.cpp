#include "api.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/vector.h>
#include <library/cpp/resource/resource.h>

using namespace NBASS::NWeather;

NAlice::TDateTime GetDate(size_t timestamp) {
    return NAlice::TDateTime(NAlice::TDateTime::TSplitTime(NDatetime::GetTimeZone("UTC"), timestamp));
}

static const size_t FIRST_DAY = 1561507200; // 2019-06-26T00:00:00+00:00
static const size_t LAST_DAY = 1562371200; // 2019-07-06T00:00:00+00:00

Y_UNIT_TEST_SUITE(WeatherApi) {
    Y_UNIT_TEST(Yesterday) {
        TForecast forecast(NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json")));
        UNIT_ASSERT_EQUAL(forecast.Yesterday.Temp, 22);
    }

    Y_UNIT_TEST(ForecastsStructure) {
        TForecast forecast(NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json")));
        UNIT_ASSERT_EQUAL(forecast.Days.size(), 11);

        for (size_t i = 0; i < forecast.Days.size() - 1; ++i) {
            UNIT_ASSERT_EQUAL(forecast.Days[i].Next, &forecast.Days[i + 1]);
        }
        UNIT_ASSERT_EQUAL(forecast.Days[forecast.Days.size() - 1].Next, nullptr);

        for (auto& day: forecast.Days) {
            UNIT_ASSERT_EQUAL(day.Parts.Morning.Day, &day);
            UNIT_ASSERT_EQUAL(day.Parts.Day.Day, &day);
            UNIT_ASSERT_EQUAL(day.Parts.Evening.Day, &day);
            UNIT_ASSERT_EQUAL(day.Parts.Night.Day, &day);
            UNIT_ASSERT_EQUAL(day.Parts.NightShort.Day, &day);
            UNIT_ASSERT_EQUAL(day.Parts.DayShort.Day, &day);
        }

        size_t hours_count = 0;
        THour hour = forecast.Today().Hours[0];
        while (hour.Next) {
            UNIT_ASSERT(hour.DayPart);
            hour = *hour.Next;
            hours_count++;
        }

        UNIT_ASSERT_EQUAL(hours_count, 84);
    }

    Y_UNIT_TEST(FindDay) {
        TForecast forecast(NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json")));
        UNIT_ASSERT(forecast.FindDay(GetDate(FIRST_DAY - 24 * 60 * 60)).Empty());
        UNIT_ASSERT(forecast.FindDay(GetDate(FIRST_DAY)).Defined());
        UNIT_ASSERT(forecast.FindDay(GetDate(LAST_DAY)).Defined());
        UNIT_ASSERT(forecast.FindDay(GetDate(LAST_DAY + 24 * 60 * 60)).Empty());

        auto day_one = forecast.FindDay(GetDate(FIRST_DAY));
        auto day_one_plus_hour = forecast.FindDay(GetDate(FIRST_DAY + 3600));
        UNIT_ASSERT_EQUAL(day_one->Date.ToString("%F"), day_one_plus_hour->Date.ToString("%F"));
    }

    Y_UNIT_TEST(FindHour) {
        TForecast forecast(NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json")));
        UNIT_ASSERT(forecast.FindHour(GetDate(FIRST_DAY - 24 * 60 * 60)).Empty());
        UNIT_ASSERT(forecast.FindHour(GetDate(FIRST_DAY)).Defined());
        UNIT_ASSERT(forecast.FindHour(GetDate(LAST_DAY)).Empty()); // No hours after 4 days
        UNIT_ASSERT(forecast.FindHour(GetDate(LAST_DAY + 24 * 60 * 60)).Empty());
    }

    Y_UNIT_TEST(TZInfoTimezoneForMoscow) {
        auto forecastJSON = NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json"));

        TForecast forecast(forecastJSON);
        UNIT_ASSERT_STRINGS_EQUAL(NDatetime::GetTimeZone(forecast.Info.TzInfo.TimeZoneName()).name(), "Europe/Moscow");
    }

    Y_UNIT_TEST(TZInfoUTCTimezoneWithPositiveOffset) {
        auto forecastJSON = NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json"));
        forecastJSON["info"]["tzinfo"]["name"] = "UTC+4";
        forecastJSON["info"]["tzinfo"]["offset"] = 4 * 60 * 60;

        TForecast forecast(forecastJSON);
        UNIT_ASSERT_STRINGS_EQUAL(NDatetime::GetTimeZone(forecast.Info.TzInfo.TimeZoneName()).name(), "Etc/GMT-4");
    }

    Y_UNIT_TEST(TZInfoUTCTimezoneWithNegativeOffset) {
        auto forecastJSON = NSc::TValue::FromJson(NResource::Find("weather_api_forecast.json"));
        forecastJSON["info"]["tzinfo"]["name"] = "UTC-01";
        forecastJSON["info"]["tzinfo"]["offset"] = -1 * 60 * 60;

        TForecast forecast(forecastJSON);
        UNIT_ASSERT_STRINGS_EQUAL(NDatetime::GetTimeZone(forecast.Info.TzInfo.TimeZoneName()).name(), "Etc/GMT+1");
    }
}
