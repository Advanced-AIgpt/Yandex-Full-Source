#include "artist_parser.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/library/json/json.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/ut/data";
constexpr TStringBuf POSITIVE_FILENAME = "artist-positive.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(ArtistParserTest) {

Y_UNIT_TEST(Positive) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();

    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto config = CreateMusicConfig({});
    mq.SetConfig(config);

    mq.InitPlayback({}, rng);
    ParseArtist(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetName(), "Madonna");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), 330);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), config.PageSize);
    TStringBuf trackIds[] = {"133207", "133054", "216187", "125949", "215618", "215892", "3619283", "178352",
                             "178412", "370847", "148122", "125906", "436979", "171203", "215444", "10072197",
                             "148115", "21243954", "21243957", "403041"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), config.PageSize);
    for (i32 i = 0; i < config.PageSize; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i));
        mq.ChangeToNextTrack();
    }
}

Y_UNIT_TEST(PositiveWithOffset) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();

    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto config = CreateMusicConfig({});
    mq.SetConfig(config);

    const size_t offset = 3;

    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetTrackOffsetIndex(offset);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseArtist(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetName(), "Madonna");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), 330);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), config.PageSize - offset);
    TStringBuf trackIds[] = {"125949", "215618", "215892", "3619283", "178352",
                             "178412", "370847", "148122", "125906", "436979", "171203", "215444", "10072197",
                             "148115", "21243954", "21243957", "403041"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), config.PageSize - offset);
    for (ui32 i = 0; i < config.PageSize - offset; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i + offset));
        mq.ChangeToNextTrack();
    }
}

Y_UNIT_TEST(PositiveWithOffsetAndHistory) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    mCtx.SetFirstRequestPageSize(20);
    auto scState = *mCtx.MutableScenarioState();

    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    auto config = CreateMusicConfig({});
    mq.SetConfig(config);

    const size_t offset = 3;

    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetTrackOffsetIndex(offset);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseArtist(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetName(), "Madonna");
    TStringBuf trackIds[] = {"133207", "133054", "216187"};
    for (i32 i = offset - 1; i >= 0; i--) {
        mq.ChangeToPrevTrack();
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), trackIds[i]);
    }
}

}

} //namespace NAlice::NHollywood::NMusic
