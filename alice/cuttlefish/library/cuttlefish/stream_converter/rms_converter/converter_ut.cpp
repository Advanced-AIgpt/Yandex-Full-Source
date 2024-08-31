#include "converter.h"

#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/ymath.h>

#include <type_traits>


using namespace NAlice::NCuttlefish::NAppHostServices::NRmsConverter;


namespace {

    void MakeChannel(
        NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData* channel
    ) {
        Y_UNUSED(channel);
    }

    template <typename... TArgs>
    void MakeChannel(
        NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData* channel,
        double head,
        TArgs... tail
    ) {
        channel->AddValues(head);
        MakeChannel(channel, tail...);
    }

    template <typename... TArgs, typename = std::enable_if_t<(std::is_arithmetic_v<TArgs> && ...)>>
    NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData MakeChannel(TArgs... values) {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TChannelRmsData channel;
        MakeChannel(&channel, values...);
        return channel;
    }

}  // anonymous namespace


class TCalcAvgRMSTest: public TTestBase {
    UNIT_TEST_SUITE(TCalcAvgRMSTest);
    UNIT_TEST(TestRawAvgRmsCorrectionForFirstMini);
    UNIT_TEST(TestRawAvgRmsCorrectionForFirstMiniWithCustomConfig);
    UNIT_TEST(TestRawAvgRmsCorrectionForSecondMini);
    UNIT_TEST(TestRawAvgRmsCorrectionForSecondMiniWithCustomConfig);
    UNIT_TEST(TestRawAvgRmsCorrectionForFirstStation);
    UNIT_TEST(TestRawAvgRmsCorrectionForFirstStationWithCustomConfig);
    UNIT_TEST(TestRawAvgRmsCorrectionForSecondStation);
    UNIT_TEST(TestRawAvgRmsCorrectionForSecondStationWithCustomConfig);
    UNIT_TEST(TestRawAvgRmsCorrectionForJblPortable);
    UNIT_TEST(TestRawAvgRmsCorrectionForJblPortableWithCustomConfig);
    UNIT_TEST(TestRawAvgRmsCorrectionForUnknownDevice);
    UNIT_TEST(TestRawAvgRmsCorrectionForUnknownDeviceWithCustomConfig);
    UNIT_TEST(TestNoCorrectionForV1PayloadWithoutRaw);
    UNIT_TEST(TestNoCorrectionForV1PayloadWithoutRawWithCustomConfig);
    UNIT_TEST(TestCalcAvgForV1PayloadWithChanelsOnly);
    UNIT_TEST(TestCalcAvgForV1PayloadWithChanelsOnlyWithCustomConfig);
    UNIT_TEST(TestZeroForV1PayloadWithInvalidVersion);
    UNIT_TEST(TestZeroForV1PayloadWithInvalidVersionWithCustomConfig);
    UNIT_TEST(TestCalcAvgForV0Payload);
    UNIT_TEST(TestCalcAvgForV0PayloadWithCustomConfig);
    UNIT_TEST_SUITE_END();

private:
    TString RmsPerDeviceConfig = R"({
        "jbl_link_music": 5.843,
        "jbl_link_portable": 5.843,
        "yandexmicro": 2.0,
        "yandexmini": 2.0,
        "yandexmini_2": 4.5,
        "yandexstation": 4.275,
        "yandexstation_2": 4.275,
        "station": 4.275,
        "station_2": 4.275,
        "yandexmidi": 3.76
    })";

public:
    void TestRawAvgRmsCorrectionForFirstMini() {
        // No correction for yandexmini
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(123.4);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "yandexmini"), 123.4));
    }

    void TestRawAvgRmsCorrectionForFirstMiniWithCustomConfig() {
        // No correction for yandexmini
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(123.4);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "yandexmini", RmsPerDeviceConfig), 246.8));
    }

    void TestRawAvgRmsCorrectionForSecondMini() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(7.9);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "yandexmini_2"), 7.9 * 3.5));
    }

    void TestRawAvgRmsCorrectionForSecondMiniWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(7.9);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "yandexmini_2", RmsPerDeviceConfig), 7.9 * 4.5));
    }

    void TestRawAvgRmsCorrectionForFirstStation() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(6.8);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station"), 6.8 * 3.275));
    }

    void TestRawAvgRmsCorrectionForFirstStationWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(6.8);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station", RmsPerDeviceConfig), 6.8 * 4.275));
    }

    void TestRawAvgRmsCorrectionForSecondStation() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(45.5);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station_2"), 45.5 * 3.275));
    }

    void TestRawAvgRmsCorrectionForSecondStationWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(45.5);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station_2", RmsPerDeviceConfig), 45.5 * 4.275));
    }

    void TestRawAvgRmsCorrectionForJblPortable() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(90.9);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "jbl_link_portable"), 90.9 * 4.843));
    }

    void TestRawAvgRmsCorrectionForJblPortableWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(90.9);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "jbl_link_portable", RmsPerDeviceConfig), 90.9 * 5.843));
    }

    void TestRawAvgRmsCorrectionForUnknownDevice() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(666);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "abyrvalg"), 666));
    }

    void TestRawAvgRmsCorrectionForUnknownDeviceWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetRawAvgRMS(666);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "abyrvalg", RmsPerDeviceConfig), 666));
    }

    void TestNoCorrectionForV1PayloadWithoutRaw() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetAvgRMS(22);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station"), 22));
    }

    void TestNoCorrectionForV1PayloadWithoutRawWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetAvgRMS(22);
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station", RmsPerDeviceConfig), 22));
    }

    void TestCalcAvgForV1PayloadWithChanelsOnly() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetVersion(1);
        features.MutableVer1()->MutableRmsData()->insert({"1", MakeChannel(100, 200, 300)});  // avg=200
        features.MutableVer1()->MutableRmsData()->insert({"2", MakeChannel(400, 600)});  // avg=500
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station"), 3.5));
    }

    void TestCalcAvgForV1PayloadWithChanelsOnlyWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetVersion(1);
        features.MutableVer1()->MutableRmsData()->insert({"1", MakeChannel(100, 200, 300)});  // avg=200
        features.MutableVer1()->MutableRmsData()->insert({"2", MakeChannel(400, 600)});  // avg=500
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station", RmsPerDeviceConfig), 3.5));
    }

    void TestZeroForV1PayloadWithInvalidVersion() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetVersion(2);
        features.MutableVer1()->MutableRmsData()->insert({"1", MakeChannel(1, 3)});  // avg=2
        features.MutableVer1()->MutableRmsData()->insert({"2", MakeChannel(4, 5, 6)});  // avg=5
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station"), 0));
    }

    void TestZeroForV1PayloadWithInvalidVersionWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        features.MutableVer1()->SetVersion(2);
        features.MutableVer1()->MutableRmsData()->insert({"1", MakeChannel(1, 3)});  // avg=2
        features.MutableVer1()->MutableRmsData()->insert({"2", MakeChannel(4, 5, 6)});  // avg=5
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station", RmsPerDeviceConfig), 0));
    }

    void TestCalcAvgForV0Payload() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        MakeChannel(features.MutableVer0()->AddRmsData(), 10, 20, 30);  // avg=20
        MakeChannel(features.MutableVer0()->AddRmsData(), 40, 50, 60);  // avg=50
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station"), 35));
    }

    void TestCalcAvgForV0PayloadWithCustomConfig() {
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures features;
        MakeChannel(features.MutableVer0()->AddRmsData(), 10, 20, 30);  // avg=20
        MakeChannel(features.MutableVer0()->AddRmsData(), 40, 50, 60);  // avg=50
        UNIT_ASSERT(FuzzyEquals(CalcAvgRMS(features, "station", RmsPerDeviceConfig), 35));
    }
};

class TConvertSpotterFeaturesTest: public TTestBase {
    UNIT_TEST_SUITE(TConvertSpotterFeaturesTest);
    UNIT_TEST(TestV0);
    UNIT_TEST(TestV1);
    UNIT_TEST_SUITE_END();

public:
    void TestV0() {
        const NJson::TJsonValue spotterRms = ReadJson("[[1, 2, 3, 4], [5, 6], \"string\", [7], []]");
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures output;
        ConvertSpotterFeatures(spotterRms, &output);
        UNIT_ASSERT(output.HasVer0());
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData().size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[0].GetValues().size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[1].GetValues().size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[2].GetValues().size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[3].GetValues().size(), 0);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[0].GetValues()[0], 1);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[0].GetValues()[1], 2);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[0].GetValues()[2], 3);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[0].GetValues()[3], 4);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[1].GetValues()[0], 5);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[1].GetValues()[1], 6);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer0().GetRmsData()[2].GetValues()[0], 7);
    }

    void TestV1() {
        const NJson::TJsonValue spotterRms = ReadJson(
            R"({
                "version": 9,
                "AvgRMS": 12.34,
                "RawAvgRMS": 43.21,
                "channels": [
                    {
                        "name": "ch1",
                        "data": [1, 2, 3, 4]
                    },
                    {
                        "name": "ch2",
                        "data": [5, 7]
                    }
                ]
            })"
        );
        NAliceProtocol::TRequestContext::TAdditionalOptions::TSpotterFeatures output;
        ConvertSpotterFeatures(spotterRms, &output);
        UNIT_ASSERT(output.HasVer1());
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetVersion(), 9);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetAvgRMS(), 12.34);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRawAvgRMS(), 43.21);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch1").GetValues().size(), 4);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch1").GetValues()[0], 1);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch1").GetValues()[1], 2);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch1").GetValues()[2], 3);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch1").GetValues()[3], 4);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch2").GetValues().size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch2").GetValues()[0], 5);
        UNIT_ASSERT_VALUES_EQUAL(output.GetVer1().GetRmsData().at("ch2").GetValues()[1], 7);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TCalcAvgRMSTest)
UNIT_TEST_SUITE_REGISTRATION(TConvertSpotterFeaturesTest)
