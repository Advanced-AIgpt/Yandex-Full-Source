#include "music_request_builder.h"

#include <library/cpp/regex/pcre/regexp.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

Y_UNIT_TEST_SUITE(MusicRequestBuilderSuite) {

Y_UNIT_TEST(Test) {
    NScenarios::TRequestMeta meta;
    meta.SetRequestId("dummy-request-id-for-tests");

    TClientInfoProto clientInfoProto;
    clientInfoProto.SetDeviceManufacturer("Samsung");
    clientInfoProto.SetDeviceModel("SM—G800F");
    clientInfoProto.SetPlatform("Android");
    clientInfoProto.SetOsVersion("4.0");
    clientInfoProto.SetDeviceId("rab6834689br63478bsir64");
    clientInfoProto.SetUuid("2db34bdjk5l34b534l");
    const TClientInfo clientInfo{clientInfoProto};

    const auto& result = TMusicRequestBuilder("/path", meta, clientInfo, TRTLogger::NullLogger(), /*enableCrossDc = */ false, {}).Build();
    UNIT_ASSERT_STRINGS_EQUAL(result.GetPath(), "/path");

    THashMap<TString, TString> headers;
    for (const auto& header : result.GetHeaders()) {
        headers[header.GetName()] = header.GetValue();
    }

    UNIT_ASSERT_EQUAL_C(headers.size(), 3, "Number of headers is " + ToString(headers.size()));

    UNIT_ASSERT_STRING_CONTAINS(headers["X-Request-Id"], "dummy-request-id-for-tests");

    UNIT_ASSERT_C(TRegExMatch(R"(AliceMusicThinClient/\d+)").Match(headers["X-Yandex-Music-Client"].data()),
                  "Couldn't match " + headers["X-Yandex-Music-Client"]);

    UNIT_ASSERT_STRINGS_EQUAL(
        headers["X-Yandex-Music-Device"],
        "os=android; os_version=4.0; manufacturer=Samsung; model=SM—G800F; clid=0; device_id=rab6834689br63478bsir64; uuid=2db34bdjk5l34b534l");
}

Y_UNIT_TEST(TestBalancingHint) {
    THttpProxyRequestBuilder::SetCurrentDc("sas");

    NScenarios::TRequestMeta meta;
    const TClientInfoProto clientInfoProto;
    const TClientInfo clientInfo{clientInfoProto};
    meta.SetRequestId("dummy-request-id-for-tests");
    const auto& result = TMusicRequestBuilder("/path", meta, clientInfo, TRTLogger::NullLogger(), /*enableCrossDc = */ false, {}).Build();

    THashMap<TString, TString> headers;
    for (const auto& header : result.GetHeaders()) {
        headers[header.GetName()] = header.GetValue();
    }

    UNIT_ASSERT_EQUAL_C(headers.size(), 3, "Number of headers is " + ToString(headers.size()));
}

Y_UNIT_TEST(TestGuestOAuthTokenSetting) {
    THttpProxyRequestBuilder::SetCurrentDc("sas");
    
    NScenarios::TRequestMeta meta;
    const TClientInfoProto clientInfoProto;
    const TClientInfo clientInfo{clientInfoProto};

    meta.SetOAuthToken("DummyOAuthTokenForTests");
    meta.SetRequestId("dummy-request-id-for-tests");

    TString guestOAuthToken = "DummyGuestOAuthTokenForTests";

    const auto& result = TMusicRequestBuilder("/path",
                                              MakeAtomicShared<TGuestRequestMetaProvider>(meta, std::move(guestOAuthToken)),
                                              clientInfo, TRTLogger::NullLogger(),
                                              /*enableCrossDc = */ false, {})
            .SetUseOAuth()
            .Build();

    THashMap<TString, TString> headers;
    for (const auto& header : result.GetHeaders()) {
        headers[header.GetName()] = header.GetValue();
    }

    UNIT_ASSERT_EQUAL_C(headers.size(), 4, "Number of headers is " + ToString(headers.size()));

    UNIT_ASSERT_STRINGS_EQUAL(headers["authorization"], "OAuth DummyGuestOAuthTokenForTests");
}

}

} // namespace NAlice::NHollywood::NMusic
