#include "track_url_builder.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/system/env.h>

namespace NAlice::NHollywood::NMusic {

namespace {
const inline TString SECRET_ENV_KEY = TString(TStringBuf("MUSICKIT_SECRET_KEY"));
constexpr TStringBuf TEST_SECRET = "test_secret";
constexpr TStringBuf REFERENCE_URL =
    "https://host.domain.com/get-mp3/6720702cb3dab08ff017e3fa25bc8c42/12345some-string?track-id=4321&from=from-value&play=false&uid=321321321";
} // namespace


Y_UNIT_TEST_SUITE(UrlBuilderTest) {

Y_UNIT_TEST(Check) {
    TXmlRespParseResult result{
        "host.domain.com", // host
        "some-string", // path
        "12345", // ts
        "-1", // region
        "abcderfghijklmn", // signature
    };
    SetEnv(SECRET_ENV_KEY, TString(TEST_SECRET));
    auto url = BuildTrackUrl("4321", "from-value", "321321321", result);
    UNIT_ASSERT_EQUAL(url, REFERENCE_URL);
}
};

}  // namespace NAlice::NHollywood::NMusic
