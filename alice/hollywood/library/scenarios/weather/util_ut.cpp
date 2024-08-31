#include <alice/hollywood/library/scenarios/weather/util/util.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NWeather {

Y_UNIT_TEST_SUITE(Weather) {
    Y_UNIT_TEST(TestNewFrameLegal) {
        // from unknown (or in new session)
        UNIT_ASSERT(IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(TStringBuf("some.frame"), NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER__DETAILS
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__DETAILS, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER__ELLIPSIS
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_NOWCAST
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_NOWCAST__ELLIPSIS
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_WIND
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_WIND__ELLIPSIS
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_WIND__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_NOWCAST_PREC_MAP
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));
    
        // from GET_WEATHER_PRESSURE
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));

        // from GET_WEATHER_PRESSURE__ELLIPSIS
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER__DETAILS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(!IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(IsNewFrameLegal(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS, NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));
    }

    Y_UNIT_TEST(TestIsTakeSlotsFromPrevFrame) {
        UNIT_ASSERT(!IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER));
        UNIT_ASSERT(!IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(!IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_WIND));
        UNIT_ASSERT(IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_WIND__ELLIPSIS));
        UNIT_ASSERT(!IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP));
        UNIT_ASSERT(IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_NOWCAST_PREC_MAP__ELLIPSIS));
        UNIT_ASSERT(!IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_PRESSURE));
        UNIT_ASSERT(IsTakeSlotsFromPrevFrame(NFrameNames::GET_WEATHER_PRESSURE__ELLIPSIS));
    }

    Y_UNIT_TEST(TestFillFromPrevFrame) {
        TFrame prevFrame{"alice.test.frame"};
        prevFrame.AddSlot(TSlot{.Name = "name1", .Type = "type1", .Value = TSlot::TValue{"value1"}});
        prevFrame.AddSlot(TSlot{.Name = "name2", .Type = "type2", .Value = TSlot::TValue{"value2"}});
        prevFrame.AddSlot(TSlot{.Name = "name3", .Type = "type3", .Value = TSlot::TValue{"value3"}});
        prevFrame.AddSlot(TSlot{.Name = "name4", .Type = "type4", .Value = TSlot::TValue{"value4"}});

        TFrame frame{"alice.test.frame.new"};
        frame.AddSlot(TSlot{.Name = "name1", .Type = "type1_new", .Value = TSlot::TValue{"value1_new"}});
        frame.AddSlot(TSlot{.Name = "name3", .Type = "type3_new", .Value = TSlot::TValue{"value3_new"}});
        frame.AddSlot(TSlot{.Name = "name5", .Type = "type5_new", .Value = TSlot::TValue{"value5_new"}});

        FillFromPrevFrame(TRTLogger::NullLogger(), prevFrame, frame);

        UNIT_ASSERT_VALUES_EQUAL(frame.Name(), "alice.test.frame.new");

        // name1 not changed
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name1")->Name, "name1");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name1")->Type, "type1_new");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name1")->Value.AsString(), "value1_new");

        // name2 copied
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name2")->Name, "name2");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name2")->Type, "type2");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name2")->Value.AsString(), "value2");

        // name3 not changed
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name3")->Name, "name3");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name3")->Type, "type3_new");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name3")->Value.AsString(), "value3_new");

        // name4 copied
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name4")->Name, "name4");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name4")->Type, "type4");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name4")->Value.AsString(), "value4");

        // name5 not changed
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name5")->Name, "name5");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name5")->Type, "type5_new");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("name5")->Value.AsString(), "value5_new");
    }

    Y_UNIT_TEST(TestGetFrameName) {
        TWeatherState state;
        UNIT_ASSERT_VALUES_EQUAL(GetFrameName(state), "");

        state.MutableSemanticFrame()->SetName("alice.test.frame");
        UNIT_ASSERT_VALUES_EQUAL(GetFrameName(state), "alice.test.frame");
    }

    Y_UNIT_TEST(TestGetWeatherForecastUri) {
        TFrame frame{"alice.test.frame"};
        UNIT_ASSERT(GetWeatherForecastUri(frame).Empty());

        frame.AddSlot(TSlot{.Name = "weather_forecast", .Type = "forecast", .Value = TSlot::TValue{"broken_value"}});
        UNIT_ASSERT(GetWeatherForecastUri(frame).Empty());

        TString weatherForecast = R"(
        {
            "date":"2020-10-01",
            "day_part":"night",
            "uri":"https://yandex.ru/pogoda?from=alice_weathercard&lat=59.938951&lon=30.315635&utm_source=alice&utm_campaign=card",
            "temperature":10,
            "type":"weather_today",
            "tz":"Europe/Moscow",
            "condition":"ясно"
        }
        )";

        const_cast<TSlot*>(frame.FindSlot("weather_forecast").Get())->Value = TSlot::TValue{std::move(weatherForecast)};
        TMaybe<TString> uri = GetWeatherForecastUri(frame);
        UNIT_ASSERT(uri.Defined());
        UNIT_ASSERT_VALUES_EQUAL(*uri, "https://yandex.ru/pogoda?from=alice_weathercard&lat=59.938951&lon=30.315635&utm_source=alice&utm_campaign=card");
    }

    Y_UNIT_TEST(TestConstructSlotRememberValue) {
        UNIT_ASSERT_VALUES_EQUAL(ConstructSlotRememberValue("test_test"), "test_test");
        UNIT_ASSERT_VALUES_EQUAL(ConstructSlotRememberValue("12345"), "12345");

        NJson::TJsonValue json;
        json["num"] = 12345;
        json["str"] = "std::string";

        NJson::TJsonArray arr;
        arr.AppendValue(678910);
        arr.AppendValue("TString");
        json["arr"] = arr;

        UNIT_ASSERT_VALUES_EQUAL(ConstructSlotRememberValue(json), "{\"arr\":[678910,\"TString\"],\"num\":12345,\"str\":\"std::string\"}");
    }

    Y_UNIT_TEST(TestIsNowcastWeatherScenario) {
        UNIT_ASSERT(!IsNowcastWeatherScenario(NFrameNames::GET_WEATHER));
        UNIT_ASSERT(!IsNowcastWeatherScenario(NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(IsNowcastWeatherScenario(NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(IsNowcastWeatherScenario(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(!IsNowcastWeatherScenario(NFrameNames::GET_WEATHER__DETAILS));
    }

    Y_UNIT_TEST(TestIsWeatherScenario) {
        UNIT_ASSERT(IsWeatherScenario(NFrameNames::GET_WEATHER));
        UNIT_ASSERT(IsWeatherScenario(NFrameNames::GET_WEATHER__ELLIPSIS));
        UNIT_ASSERT(!IsWeatherScenario(NFrameNames::GET_WEATHER_NOWCAST));
        UNIT_ASSERT(!IsWeatherScenario(NFrameNames::GET_WEATHER_NOWCAST__ELLIPSIS));
        UNIT_ASSERT(!IsWeatherScenario(NFrameNames::GET_WEATHER__DETAILS));
    }

    Y_UNIT_TEST(IsSlotEmpty) {
        UNIT_ASSERT(IsSlotEmpty(TPtrWrapper<TSlot>{nullptr, ""}));

        TSlot slot{.Name = "slot", .Type = "type", .Value = TSlot::TValue{"null"}};
        UNIT_ASSERT(IsSlotEmpty(TPtrWrapper<TSlot>{&slot, "slot"}));

        slot.Value = TSlot::TValue{"some_string"};
        UNIT_ASSERT(!IsSlotEmpty(TPtrWrapper<TSlot>{&slot, "some"}));
    }

    Y_UNIT_TEST(TestDontFillDayPartSlot) {
        // no "when" changed
        TFrame prevFrame{"alice.test.frame"};
        prevFrame.AddSlot(TSlot{.Name = "day_part", .Type = "day_part_type", .Value = TSlot::TValue{"day_part_value"}});

        TFrame frame{"alice.test.frame.new"};
        FillFromPrevFrame(TRTLogger::NullLogger(), prevFrame, frame);

        UNIT_ASSERT(frame.FindSlot("day_part"));

        // no "when" changed, had "when" previously
        prevFrame = TFrame{"alice.test.frame"};
        prevFrame.AddSlot(TSlot{.Name = "day_part", .Type = "day_part_type", .Value = TSlot::TValue{"day_part_value"}});
        prevFrame.AddSlot(TSlot{.Name = "when", .Type = "when_type", .Value = TSlot::TValue{"when_value"}});

        frame = TFrame{"alice.test.frame.new"};
        FillFromPrevFrame(TRTLogger::NullLogger(), prevFrame, frame);

        UNIT_ASSERT(frame.FindSlot("day_part"));
        UNIT_ASSERT(frame.FindSlot("when"));

        // "when" changed
        prevFrame = TFrame{"alice.test.frame"};
        prevFrame.AddSlot(TSlot{.Name = "day_part", .Type = "day_part_type", .Value = TSlot::TValue{"day_part_value"}});

        frame = TFrame{"alice.test.frame.new"};
        frame.AddSlot(TSlot{.Name = "when", .Type = "when_type", .Value = TSlot::TValue{"when_value"}});
        FillFromPrevFrame(TRTLogger::NullLogger(), prevFrame, frame);

        UNIT_ASSERT(!frame.FindSlot("day_part"));
        UNIT_ASSERT(frame.FindSlot("when"));

        // "when" changed, had "when" previously
        prevFrame = TFrame{"alice.test.frame"};
        prevFrame.AddSlot(TSlot{.Name = "day_part", .Type = "day_part_type", .Value = TSlot::TValue{"day_part_value"}});
        prevFrame.AddSlot(TSlot{.Name = "when", .Type = "old_when_type", .Value = TSlot::TValue{"old_when_value"}});

        frame = TFrame{"alice.test.frame.new"};
        frame.AddSlot(TSlot{.Name = "when", .Type = "new_when_type", .Value = TSlot::TValue{"new_when_value"}});
        FillFromPrevFrame(TRTLogger::NullLogger(), prevFrame, frame);

        UNIT_ASSERT(!frame.FindSlot("day_part"));
        UNIT_ASSERT(frame.FindSlot("when"));
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("when")->Type, "new_when_type");
        UNIT_ASSERT_VALUES_EQUAL(frame.FindSlot("when")->Value.AsString(), "new_when_value");
    }

    Y_UNIT_TEST(TestGetFrameNameFromCallbackFound) {
        const TStringBuf data = TStringBuf(R"(Name: "update_form" Payload { fields { key: "form_update" value { struct_value { fields { key: "name" value { string_value: "personal_assistant.scenarios.get_weather" } }
                                        fields { key: "push_id" value { string_value: "weather_today" } } fields { key: "slots" value { list_value { values { struct_value { fields { key: "name" value {
                                        string_value: "when" } } fields { key: "optional" value { bool_value: true } } fields { key: "source_text" value { string_value: "сейчас" } } fields { key: "type" value
                                        { string_value: "datetime" } } fields { key: "value" value { struct_value { fields { key: "seconds" value { number_value: 0 } } fields { key: "seconds_relative" value
                                        { bool_value: true } } } } } } } } } } } } } fields { key: "resubmit" value { bool_value: true } } } IsLedSilent: true)");

        const auto callback = ParseProtoText<NAlice::NScenarios::TCallbackDirective>(data);
        TMaybe<TString> frameName =  GetFrameNameFromCallback(callback);

        UNIT_ASSERT(frameName.Defined());
        UNIT_ASSERT_VALUES_EQUAL(frameName.GetRef(), "personal_assistant.scenarios.get_weather");
    }

    Y_UNIT_TEST(TestGetFrameNameFromCallbackNotFound) {
        const TStringBuf data = TStringBuf(R"(Name: "ololo" Payload { fields { key: "test" value { bool_value: true } } } IsLedSilent: true)");

        const auto callback = ParseProtoText<NAlice::NScenarios::TCallbackDirective>(data);
        TMaybe<TString> frameName =  GetFrameNameFromCallback(callback);

        UNIT_ASSERT(!frameName.Defined());
    }

    Y_UNIT_TEST(TestScledPattern) {
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(+5),      "    5*");
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(+15),     "   15*");
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(+99),     "   99*");
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(+100500), "   99*"); // impossible!

        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(0),       "    0*");

        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(-5),      "   -5*");
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(-15),     "  -15*");
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(-99),     "  -99*");
        UNIT_ASSERT_VALUES_EQUAL(ConstructScledPattern(-100500), "  -99*"); // impossible!
    }
}

} // namespace NAlice::NHollywood::NWeather
