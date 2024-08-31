#include "find_track_idx_response_parsers.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/env.h>

#include <util/stream/file.h>
#include <util/folder/path.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_backend_api/ut/data";
constexpr TStringBuf FIND_TRACK_IDX_PLAYLIST_FILENAME = "find_track_idx_playlist.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(FindTrackIdxParsersTest) {

Y_UNIT_TEST(PlaylistFoundTrackId) {
    auto data = ReadTestData(FIND_TRACK_IDX_PLAYLIST_FILENAME);
    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();
    TFindTrackIdxRequest findTrackIdxRequest;
    findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Playlist);
    findTrackIdxRequest.SetTrackId("20369522");
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    ParseFindTrackIdxResponse(data, findTrackIdxRequest, mq);
    UNIT_ASSERT_EQUAL(mq.GetTrackOffsetIndex(), 20);
}

Y_UNIT_TEST(PlaylistFoundTrackIdWithSpecificTrackOffset) {
    auto data = ReadTestData(FIND_TRACK_IDX_PLAYLIST_FILENAME);
    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();
    TFindTrackIdxRequest findTrackIdxRequest;
    findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Playlist);
    findTrackIdxRequest.SetTrackId("240214");
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    ParseFindTrackIdxResponse(data, findTrackIdxRequest, mq);
    // Original index is 44, however trackOffset is 43, because track 42 (between 40 and 44) is skipped
    UNIT_ASSERT_EQUAL(mq.GetTrackOffsetIndex(), 43);
}

Y_UNIT_TEST(PlaylistNoSuchTrackId) {
    auto data = ReadTestData(FIND_TRACK_IDX_PLAYLIST_FILENAME);
    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();
    TFindTrackIdxRequest findTrackIdxRequest;
    findTrackIdxRequest.SetContentType(TFindTrackIdxRequest_EContentType_Playlist);
    findTrackIdxRequest.SetTrackId("734158");
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    ParseFindTrackIdxResponse(data, findTrackIdxRequest, mq);
    UNIT_ASSERT_EQUAL(mq.GetTrackOffsetIndex(), 0);
}

Y_UNIT_TEST(OptimalPageParameters) {
    TMusicContext mCtx;
    auto scState = *mCtx.MutableScenarioState();
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 0), std::pair(40ul, 0ul));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 19), std::pair(40ul, 0ul));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 20), std::pair(40ul, 1ul));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 40), std::pair(30ul, 2ul));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 82), std::pair(50ul, 4ul));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 105), std::pair(30ul, 5ul));

    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 121), std::pair(35ul, 6ul));

    // First x such as 29 * 20 % x == 0 is 29 * 2 > 50. So function returns default
    UNIT_ASSERT_VALUES_EQUAL(FindOptimalPageParameters(mq, 28 * 20), std::pair(40ul, 28ul));
}

}

} //namespace NAlice::NHollywood::NMusic
