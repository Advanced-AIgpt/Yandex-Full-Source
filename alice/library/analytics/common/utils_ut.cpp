#include "names.h"
#include "utils.h"

#include <alice/library/unittest/message_diff.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

const auto BASS_RESPONSE_WITH_ANALYTICS_INFO = TStringBuf(R"(
{
    "blocks":[
        {
            "data":"QgNxd2VCA2V3cQ==",
            "type":"scenario_analytics_info"
        }
    ]
}
)");

const auto BASS_RESPONSE_WITH_INCORRECT_ANALYTICS_INFO = TStringBuf(R"(
{
    "blocks":[
        {
            "data":"abcd",
            "type":"scenario_analytics_info"
        }
    ]
}
)");

const auto EMPTY = TStringBuf(R"({})");

using namespace NAlice::NScenarios;

Y_UNIT_TEST_SUITE(ScenariosUtils) {
    Y_UNIT_TEST(GetAnalyticsInfoFromBase64) {
        auto actualAnalyticsInfo = GetAnalyticsInfoFromBase64("QgNxd2VCA2V3cQ==");

        TAnalyticsInfo expectedAnalyticsInfo;
        expectedAnalyticsInfo.AddTunnellerRawResponses("qwe");
        expectedAnalyticsInfo.AddTunnellerRawResponses("ewq");

        UNIT_ASSERT(actualAnalyticsInfo);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, *actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromEmptyBase64) {
        auto actualAnalyticsInfo = GetAnalyticsInfoFromBase64("");
        UNIT_ASSERT(!actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromEmptyIncorrectBase64) {
        auto actualAnalyticsInfo = GetAnalyticsInfoFromBase64("abcd");
        UNIT_ASSERT(!actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromBassJsonResponse) {
        auto actualAnalyticsInfo =
            GetAnalyticsInfoFromBassResponse(NJson::ReadJsonFastTree(BASS_RESPONSE_WITH_ANALYTICS_INFO));
        TAnalyticsInfo expectedAnalyticsInfo;
        expectedAnalyticsInfo.AddTunnellerRawResponses("qwe");
        expectedAnalyticsInfo.AddTunnellerRawResponses("ewq");

        UNIT_ASSERT(actualAnalyticsInfo);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, *actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromBassJsonEmptyResponse) {
        auto actualAnalyticsInfo = GetAnalyticsInfoFromBassResponse(NJson::ReadJsonFastTree(EMPTY));
        UNIT_ASSERT(!actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromBassSchemeResponse) {
        auto actualAnalyticsInfo =
            GetAnalyticsInfoFromBassResponse(NSc::TValue::FromJson(BASS_RESPONSE_WITH_ANALYTICS_INFO));
        TAnalyticsInfo expectedAnalyticsInfo;
        expectedAnalyticsInfo.AddTunnellerRawResponses("qwe");
        expectedAnalyticsInfo.AddTunnellerRawResponses("ewq");

        UNIT_ASSERT(actualAnalyticsInfo);
        UNIT_ASSERT_MESSAGES_EQUAL(expectedAnalyticsInfo, *actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromBassSchemeEmptyResponse) {
        auto actualAnalyticsInfo = GetAnalyticsInfoFromBassResponse(NSc::TValue::FromJson(EMPTY));
        UNIT_ASSERT(!actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromBassJsonIncorrectResponse) {
        auto actualAnalyticsInfo =
            GetAnalyticsInfoFromBassResponse(NJson::ReadJsonFastTree(BASS_RESPONSE_WITH_INCORRECT_ANALYTICS_INFO));
        UNIT_ASSERT(!actualAnalyticsInfo);
    }

    Y_UNIT_TEST(GetAnalyticsInfoFromBassSchemeIncorrectResponse) {
        auto actualAnalyticsInfo =
            GetAnalyticsInfoFromBassResponse(NJson::ReadJsonFastTree(BASS_RESPONSE_WITH_INCORRECT_ANALYTICS_INFO));
        UNIT_ASSERT(!actualAnalyticsInfo);
    }
}

} // namespace

namespace {

using namespace NAlice::NAnalyticsInfo;

const TStringBuf CUSTOM_PROFILE_NAME = "lolkek";

const auto CUSTOM_EXPECTED = TStringBuilder{} << NImpl::PROFILE_NAME_CGI_VALUE_PREFIX << CUSTOM_PROFILE_NAME;
const auto DEFAULT_EXPECTED = TStringBuilder{} << NImpl::PROFILE_NAME_CGI_VALUE_PREFIX << DEFAULT_TUNNELLER_PROFILE;

Y_UNIT_TEST_SUITE(AnalyticsUtils) {
    Y_UNIT_TEST(ConstructProfileNameByStringFlagDefault) {
        const TMaybe<TString> tunnellerProfileFlag;
        const auto actual = NImpl::ConstructProfileNameByFlag(tunnellerProfileFlag);
        UNIT_ASSERT_VALUES_EQUAL(DEFAULT_EXPECTED, actual);
    }

    Y_UNIT_TEST(ConstructProfileNameByStringFlagCustom) {
        const TMaybe<TString> tunnellerProfileFlag(CUSTOM_PROFILE_NAME);
        const auto actual = NImpl::ConstructProfileNameByFlag(tunnellerProfileFlag);
        UNIT_ASSERT_VALUES_EQUAL(CUSTOM_EXPECTED, actual);
    }

    Y_UNIT_TEST(ConstructWebSearchRequestCgiParametersDefault) {
        const TMaybe<TString> tunnellerProfileFlag;
        TCgiParameters expected = NImpl::COMMON_CGI_PARAMETERS;
        expected.InsertEscaped(NImpl::PROFILE_NAME_CGI_KEY, DEFAULT_EXPECTED);
        const auto actual = ConstructWebSearchRequestCgiParameters(tunnellerProfileFlag);
        UNIT_ASSERT_EQUAL(expected.size(), actual.size());
        for (const auto& [k, v] : expected) {
            UNIT_ASSERT(actual.Has(k, v));
        }
    }

    Y_UNIT_TEST(ConstructWebSearchRequestCgiParametersCustom) {
        const TMaybe<TString> tunnellerProfileFlag(CUSTOM_PROFILE_NAME);
        TCgiParameters expected = NImpl::COMMON_CGI_PARAMETERS;
        expected.InsertEscaped(NImpl::PROFILE_NAME_CGI_KEY, CUSTOM_EXPECTED);
        const auto actual = ConstructWebSearchRequestCgiParameters(tunnellerProfileFlag);
        UNIT_ASSERT_EQUAL(expected.size(), actual.size());
        for (const auto& [k, v] : expected) {
            UNIT_ASSERT(actual.Has(k, v));
        }
    }
}

} // namespace
