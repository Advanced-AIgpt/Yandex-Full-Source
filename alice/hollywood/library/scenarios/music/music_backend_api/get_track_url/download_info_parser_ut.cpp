#include "download_info_parser.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <util/stream/file.h>
#include <util/folder/path.h>
#include <library/cpp/testing/unittest/env.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/ut/data";
constexpr TStringBuf POSITIVE1_FILENAME = "dlinfo-positive.json";
constexpr TStringBuf NEGATIVE1_FILENAME = "dlinfo-negative1.json";
constexpr TStringBuf NEGATIVE2_FILENAME = "dlinfo-negative2.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(DownloadInfoParserTest) {

    Y_UNIT_TEST(Positive) {
        auto data = ReadTestData(POSITIVE1_FILENAME);
        auto opts = ParseDownloadInfo(data, TRTLogger::NullLogger());
        UNIT_ASSERT_EQUAL(opts.size() , 4);

        UNIT_ASSERT_EQUAL(opts[0].DownloadInfoUrl, "url1");
        UNIT_ASSERT_EQUAL(opts[0].Gain, true);
        UNIT_ASSERT_EQUAL(opts[0].BitrateInKbps, 64);
        UNIT_ASSERT_EQUAL(opts[0].Preview, true);
        UNIT_ASSERT_EQUAL(opts[0].Codec, EAudioCodec::AAC);
        UNIT_ASSERT_EQUAL(opts[0].ExpiringAtMs, 1653669451000);

        UNIT_ASSERT_EQUAL(opts[1].DownloadInfoUrl, "url2");
        UNIT_ASSERT_EQUAL(opts[1].Gain, false);
        UNIT_ASSERT_EQUAL(opts[1].BitrateInKbps, 192);
        UNIT_ASSERT_EQUAL(opts[1].Preview, false);
        UNIT_ASSERT_EQUAL(opts[1].Codec, EAudioCodec::MP3);
        UNIT_ASSERT_EQUAL(opts[1].ExpiringAtMs, 1653669451001);

        UNIT_ASSERT_EQUAL(opts[2].DownloadInfoUrl, "url3");
        UNIT_ASSERT_EQUAL(opts[2].Gain, false);
        UNIT_ASSERT_EQUAL(opts[2].BitrateInKbps, 192);
        UNIT_ASSERT_EQUAL(opts[2].Preview, false);
        UNIT_ASSERT_EQUAL(opts[2].Codec, EAudioCodec::AAC);
        UNIT_ASSERT_EQUAL(opts[2].ExpiringAtMs, 1653669451002);

        UNIT_ASSERT_EQUAL(opts[3].DownloadInfoUrl, "url4");
        UNIT_ASSERT_EQUAL(opts[3].Gain, true);
        UNIT_ASSERT_EQUAL(opts[3].BitrateInKbps, 128);
        UNIT_ASSERT_EQUAL(opts[3].Preview, false);
        UNIT_ASSERT_EQUAL(opts[3].Codec, EAudioCodec::AAC);
        UNIT_ASSERT_EQUAL(opts[3].ExpiringAtMs, 1653669451003);
    }

    Y_UNIT_TEST(Negative1) {
        auto data = ReadTestData(NEGATIVE1_FILENAME);
        auto opts = ParseDownloadInfo(data, TRTLogger::NullLogger());
        UNIT_ASSERT_EQUAL(opts.size() , 2);


        UNIT_ASSERT_EQUAL(opts[0].DownloadInfoUrl, "url2");
        UNIT_ASSERT_EQUAL(opts[0].Gain, false);
        UNIT_ASSERT_EQUAL(opts[0].BitrateInKbps, 192);
        UNIT_ASSERT_EQUAL(opts[0].Preview, false);
        UNIT_ASSERT_EQUAL(opts[0].Codec, EAudioCodec::MP3);
        UNIT_ASSERT_EQUAL(opts[0].ExpiringAtMs, 0);

        UNIT_ASSERT_EQUAL(opts[1].DownloadInfoUrl, "url4");
        UNIT_ASSERT_EQUAL(opts[1].Gain, true);
        UNIT_ASSERT_EQUAL(opts[1].BitrateInKbps, 128);
        UNIT_ASSERT_EQUAL(opts[1].Preview, false);
        UNIT_ASSERT_EQUAL(opts[1].Codec, EAudioCodec::AAC);
        UNIT_ASSERT_EQUAL(opts[1].ExpiringAtMs, 0);
    }

    Y_UNIT_TEST(Negative2) {
        auto data = ReadTestData(NEGATIVE2_FILENAME);
        UNIT_ASSERT_EXCEPTION(ParseDownloadInfo(data, TRTLogger::NullLogger()), yexception);
    }

}

} //namespace NAlice::NHollywood::NMusic
