#include "track_parser.h"

#include <alice/library/json/json.h>
#include <alice/protos/data/scenario/music/content_info.pb.h>
#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/folder/path.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf TEST_DATA_DIR =
    "alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/ut/data";
constexpr TStringBuf TRACK_POSITIVE_FILENAME = "track-positive.json";
constexpr TStringBuf TRACK_R128_FILENAME = "track-r128.json";
constexpr TStringBuf TRACK_CHILD_CONTENT = "track_child_content.json";
constexpr TStringBuf TRACK_IS_SUITABLE_FOR_CHILDREN = "track_is_suitable_for_children.json";
constexpr TStringBuf TRACK_EXPLICIT = "track_explicit.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(TrackParserTest) {

Y_UNIT_TEST(Positive) {
    auto data = ReadTestData(TRACK_POSITIVE_FILENAME);
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    mq.InitPlayback({}, rng);
    ParseSingleTrack(JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();
    const auto& track = mq.CurrentItem();
    const auto& trackInfo = track.GetTrackInfo();
    UNIT_ASSERT_STRINGS_EQUAL(track.GetTrackId(), "133207");
    UNIT_ASSERT_STRINGS_EQUAL(trackInfo.GetAlbumId(), "18198");
    UNIT_ASSERT_STRINGS_EQUAL(track.GetTitle(), "4 Minutes");
    UNIT_ASSERT_STRINGS_EQUAL(TMusicQueueWrapper::ArtistName(track), "Madonna");
    UNIT_ASSERT_STRINGS_EQUAL(trackInfo.GetArtistId(), "1813");
    UNIT_ASSERT_STRINGS_EQUAL(trackInfo.GetAlbumTitle(), "Hard Candy");
    UNIT_ASSERT_STRINGS_EQUAL(track.GetCoverUrl(), "avatars.yandex.net/get-music-content/28589/872e4153.a.18198-1/%%");
    UNIT_ASSERT_VALUES_EQUAL(track.GetDurationMs(), 246260);
    UNIT_ASSERT_STRINGS_EQUAL(trackInfo.GetGenre(), "pop");
    UNIT_ASSERT_STRINGS_EQUAL(trackInfo.GetAlbumType(), "");
    UNIT_ASSERT_STRINGS_EQUAL(track.GetType(), "music");
    UNIT_ASSERT_STRINGS_EQUAL(trackInfo.GetAlbumCoverUrl(), "avatars.yandex.net/get-music-content/28589/872e4153.a.18198-1/%%");
    UNIT_ASSERT(!track.HasNormalization());

    // check track artists
    UNIT_ASSERT_VALUES_EQUAL(trackInfo.ArtistsSize(), 3);
    const auto& artists0 = trackInfo.GetArtists(0);
    UNIT_ASSERT_STRINGS_EQUAL(artists0.GetId(), "1813");
    UNIT_ASSERT_STRINGS_EQUAL(artists0.GetName(), "Madonna");
    UNIT_ASSERT_VALUES_EQUAL(artists0.GetComposer(), false);
    UNIT_ASSERT_VALUES_EQUAL(artists0.GetVarious(), false);

    const auto& artists1 = trackInfo.GetArtists(1);
    UNIT_ASSERT_STRINGS_EQUAL(artists1.GetId(), "1151");
    UNIT_ASSERT_STRINGS_EQUAL(artists1.GetName(), "Justin Timberlake");
    UNIT_ASSERT_VALUES_EQUAL(artists1.GetComposer(), false);
    UNIT_ASSERT_VALUES_EQUAL(artists1.GetVarious(), false);

    const auto& artists2 = trackInfo.GetArtists(2);
    UNIT_ASSERT_STRINGS_EQUAL(artists2.GetId(), "1156");
    UNIT_ASSERT_STRINGS_EQUAL(artists2.GetName(), "Timbaland");
    UNIT_ASSERT_VALUES_EQUAL(artists2.GetComposer(), false);
    UNIT_ASSERT_VALUES_EQUAL(artists2.GetVarious(), false);

    // check album artists
    UNIT_ASSERT_VALUES_EQUAL(trackInfo.AlbumArtistsSize(), 1);
    const auto& albumArtists0 = trackInfo.GetAlbumArtists(0);
    UNIT_ASSERT_STRINGS_EQUAL(albumArtists0.GetId(), "1813");
    UNIT_ASSERT_STRINGS_EQUAL(albumArtists0.GetName(), "Madonna");
    UNIT_ASSERT_VALUES_EQUAL(albumArtists0.GetComposer(), false);
    UNIT_ASSERT_VALUES_EQUAL(albumArtists0.GetVarious(), false);

    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentInfo().GetTitle(), "4 Minutes");
    UNIT_ASSERT(mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());
    UNIT_ASSERT_EQUAL(track.GetContentWarning(), EContentWarning::Unknown);
}

Y_UNIT_TEST(R128) {
    auto data = ReadTestData(TRACK_R128_FILENAME);
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    mq.InitPlayback({}, rng);
    ParseSingleTrack(JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();
    const auto& track = mq.CurrentItem();
    UNIT_ASSERT(track.HasNormalization());
    UNIT_ASSERT_DOUBLES_EQUAL(track.GetNormalization().GetIntegratedLoudness(), -9.37, 0.001);
    UNIT_ASSERT_DOUBLES_EQUAL(track.GetNormalization().GetTruePeak(), 0.86, 0.001);

    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentInfo().GetTitle(), "Tuff Chat");
    UNIT_ASSERT(mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());
    UNIT_ASSERT_EQUAL(track.GetContentWarning(), EContentWarning::Unknown);
}

Y_UNIT_TEST(ChildContent) {
    auto data = ReadTestData(TRACK_CHILD_CONTENT);
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    mq.InitPlayback({}, rng);
    ParseSingleTrack(JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentInfo().GetTitle(), "Tuff Chat");
    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    const auto& track = mq.CurrentItem();
    UNIT_ASSERT_EQUAL(track.GetContentWarning(), EContentWarning::ChildSafe);
}

Y_UNIT_TEST(IsSuitableForChildren) {
    auto data = ReadTestData(TRACK_IS_SUITABLE_FOR_CHILDREN);
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    mq.InitPlayback({}, rng);
    ParseSingleTrack(JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentInfo().GetTitle(), "Tuff Chat");
    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    const auto& track = mq.CurrentItem();
    UNIT_ASSERT_EQUAL(track.GetContentWarning(), EContentWarning::ChildSafe);
}

Y_UNIT_TEST(Explicit) {
    auto data = ReadTestData(TRACK_EXPLICIT);
    TScenarioState scState;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));
    mq.InitPlayback({}, rng);
    ParseSingleTrack(JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentInfo().GetTitle(), "Tuff Chat");
    UNIT_ASSERT(mq.HaveNonChildSafeContent());
    UNIT_ASSERT(mq.HaveExplicitContent());

    const auto& track = mq.CurrentItem();
    UNIT_ASSERT_EQUAL(track.GetContentWarning(), EContentWarning::Explicit);
}

}

} //namespace NAlice::NHollywood::NMusic
