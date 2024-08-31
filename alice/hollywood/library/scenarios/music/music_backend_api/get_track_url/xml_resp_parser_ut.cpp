#include "xml_resp_parser.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <util/stream/file.h>
#include <util/folder/path.h>
#include <library/cpp/testing/unittest/env.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/ut/data";
constexpr TStringBuf POSITIVE1_FILENAME = "positive1.xml";
constexpr TStringBuf POSITIVE2_FILENAME = "positive2.xml";
constexpr TStringBuf NEGATIVE1_FILENAME = "negative1.xml";
constexpr TStringBuf NEGATIVE_NO_HOST_FILENAME = "negative_no_host.xml";
constexpr TStringBuf NEGATIVE_NO_PATH_FILENAME = "negative_no_path.xml";
constexpr TStringBuf NEGATIVE_NO_TS_FILENAME = "negative_no_ts.xml";
constexpr TStringBuf NEGATIVE_NO_S_FILENAME = "negative_no_s.xml";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(XmlRespParserTest) {

Y_UNIT_TEST(Positive) {
    {
        auto xml = ReadTestData(POSITIVE1_FILENAME);
        auto maybeResult = ParseDlInfoXmlResp(xml);
        UNIT_ASSERT(maybeResult);
        UNIT_ASSERT_EQUAL(maybeResult->Signature, "df4dd51823e5bec643e62a3e2b1ca91acd622d849e71a52cd658a757c1f4be97");
        UNIT_ASSERT_EQUAL(maybeResult->Host, "s174sas.storage.yandex.net");
        UNIT_ASSERT_EQUAL(maybeResult->Path, "/rmusic/U2FsdGVkX19LO");
        UNIT_ASSERT_EQUAL(maybeResult->Ts, "0005a3f1f0b59585");
        UNIT_ASSERT_EQUAL(maybeResult->Region, "-1");
    }
    {
        //human readable formatting and reordered fields
        auto xml = ReadTestData(POSITIVE2_FILENAME);
        auto maybeResult = ParseDlInfoXmlResp(xml);
        UNIT_ASSERT(maybeResult);
        UNIT_ASSERT_EQUAL(maybeResult->Signature, "df4dd51823e5bec643e62a3e2b1ca91acd622d849e71a52cd658a757c1f4be97");
        UNIT_ASSERT_EQUAL(maybeResult->Host, "s174sas.storage.yandex.net");
        UNIT_ASSERT_EQUAL(maybeResult->Path, "/rmusic/U2FsdGVkX19LO");
        UNIT_ASSERT_EQUAL(maybeResult->Ts, "0005a3f1f0b59585");
        UNIT_ASSERT_EQUAL(maybeResult->Region, "-1");
    }
}

Y_UNIT_TEST(Negative) {
    UNIT_ASSERT(!ParseDlInfoXmlResp(ReadTestData(NEGATIVE1_FILENAME)));
    UNIT_ASSERT(!ParseDlInfoXmlResp(ReadTestData(NEGATIVE_NO_HOST_FILENAME)));
    UNIT_ASSERT(!ParseDlInfoXmlResp(ReadTestData(NEGATIVE_NO_PATH_FILENAME)));
    UNIT_ASSERT(!ParseDlInfoXmlResp(ReadTestData(NEGATIVE_NO_TS_FILENAME)));
    UNIT_ASSERT(!ParseDlInfoXmlResp(ReadTestData(NEGATIVE_NO_S_FILENAME)));
}

}

} //namespace NAlice::NHollywood::NMusic
