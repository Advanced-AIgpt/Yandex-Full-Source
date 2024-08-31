#include "signature_token.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/system/env.h>

namespace NAlice::NHollywood::NMusic {

namespace {
const inline TString SECRET_ENV_KEY = TString(TStringBuf("MUSICKIT_SECRET_KEY"));
constexpr TStringBuf TEST_SECRET = "test_secret";
constexpr TStringBuf REFERENCE_SIGNATURE = "6720702cb3dab08ff017e3fa25bc8c42";
constexpr TStringBuf REFERENCE_SIGNATURE_HLS = "i8qi/rImyskNwsHWKgAL8FF4OzBAaya+pJtuHzRq0MY=";
} // namespace

Y_UNIT_TEST_SUITE(SignatureTest) {
    Y_UNIT_TEST(Check) {
        TXmlRespParseResult result{
            "host.domain.com", // host
            "some-string", // path
            "12345", // ts
            "-1", // region
            "abcderfghijklmn", // signature
        };
        auto sigToken = NImpl::CalculateSignatureToken(result, TEST_SECRET);
        UNIT_ASSERT_EQUAL(sigToken, REFERENCE_SIGNATURE);
    }
    Y_UNIT_TEST(CheckHls) {
        auto ts = TInstant::Seconds(1623212928);
        auto sigToken = NImpl::CalculateHlsSignatureToken("123321", ts, TEST_SECRET);
        UNIT_ASSERT_STRINGS_EQUAL(sigToken, REFERENCE_SIGNATURE_HLS);
    }
}

} //namespace NAlice::NHollywood::NMusic
