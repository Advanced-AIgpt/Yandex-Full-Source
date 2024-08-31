#include "playlist_parser.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/library/json/json.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
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
constexpr TStringBuf PLAYLIST_FILENAME = "playlist.json";
constexpr TStringBuf PLAYLIST_BAD_AVAILABLE_FILENAME = "playlist_bad_available.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(PlaylistParserTest) {

Y_UNIT_TEST(FilterUnavailableTracks) {
    auto data = ReadTestData(PLAYLIST_FILENAME);
    TMusicContext mCtx;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    mq.InitPlayback({}, rng);
    ParsePlaylist(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    // "title": ""
    UNIT_ASSERT(!mq.ContentInfo().HasTitle() && !mq.ContentInfo().HasName());

    // there are 20 tracks in playlist.json
    // every second track (id = 2, 4, 6, ...) is unavailable
    const size_t tracksCount = 10;
    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
    for (size_t i = 0; i < tracksCount; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), ToString(2 * i + 1)); // trackId = 1, 3, 5, ...
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(2 * i));
        mq.ChangeToNextTrack();
    }
}

Y_UNIT_TEST(AvailableForPremiumUsersTracks) {
    auto data = ReadTestData(PLAYLIST_BAD_AVAILABLE_FILENAME);
    TMusicContext mCtx;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    mq.InitPlayback({}, rng);
    ParsePlaylist(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "Валдай");

    const size_t tracksCount = 2;
    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
}

TMusicQueueWrapper loadContent(TMusicContext& mCtx, const TStringBuf playlistContentJson) {
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    mq.SetFiltrationMode(NScenarios::TUserPreferences_EFiltrationMode_Safe);

    mq.InitPlayback({}, rng);

    auto data = ReadTestData(playlistContentJson);
    ParsePlaylist(JsonFromString(data), mq, mCtx);

    mq.ChangeState();

    return mq;
}

Y_UNIT_TEST(PlaylistChildContent) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, "playlist_child_content.json");

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "Песни для детского праздника", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 41, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(PlaylistChildContentInRoot) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, "playlist_child_content_in_root.json");

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "Песни для детского праздника", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 41, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(PlaylistChildContentInTracks) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, "playlist_child_content_in_tracks.json");

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "Песни для детского праздника", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 41, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(PlaylistChildContentIsSuitableForChildren) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, "playlist_is_suitable_for_children.json");

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "Песни для детского праздника", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 41, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(PlaylistNeutral) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, "playlist_neutral.json");

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "Песни для детского праздника", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 41, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 0, TStringBuilder() << " but it was " << mq.QueueSize());
}

}

} //namespace NAlice::NHollywood::NMusic
