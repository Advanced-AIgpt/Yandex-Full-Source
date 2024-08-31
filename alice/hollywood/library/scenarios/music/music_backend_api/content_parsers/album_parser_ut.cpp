#include "album_parser.h"
#include "util/string/builder.h"

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
constexpr TStringBuf POSITIVE_FILENAME = "album-positive.json";
constexpr TStringBuf POSITIVE_BIG_FILENAME = "album-positive-big.json";
constexpr TStringBuf ALBUM_CHILD_CONTENT = "album_child_content.json";
constexpr TStringBuf ALBUM_CHILD_CONTENT_IN_ROOT = "album_child_content_in_root.json";
constexpr TStringBuf ALBUM_CHILD_CONTENT_IN_TRACKS = "album_child_content_in_tracks.json";
constexpr TStringBuf ALBUM_IS_SUITABLE_FOR_CHILDREN = "album_is_suitable_for_children.json";
constexpr TStringBuf ALBUM_NEUTRAL_CONTENT = "album_neutral_content.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(AlbumParserTest) {

Y_UNIT_TEST(Positive) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    mq.InitPlayback({}, rng);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    const size_t tracksCount = 10;

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "STRENGTH IN NUMB333RS");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), tracksCount);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
    TStringBuf trackIds[] = {"48737124", "44939093", "48737125", "48737126", "48737127",
                             "48737128", "48737129", "48737130", "48737131", "48737132"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), tracksCount);
    for (size_t i = 0; i < tracksCount; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i));
        mq.ChangeToNextTrack();
    }

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
}

Y_UNIT_TEST(PositiveBig1) {
    auto data = ReadTestData(POSITIVE_BIG_FILENAME);
    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig({.PageSize=31});

    mq.InitPlayback({}, rng);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    const size_t totalTracksCount = 31;
    const size_t queueTracksCount = 31;

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "The Best Of 25 Years");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), totalTracksCount);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), queueTracksCount);
    TStringBuf trackIds[] = {"2761937", "2761950", "2761954", "2761943", "545860",
                             "788179", "781328", "2761946", "788183", "788177",
                             "2761948", "2761945", "808925", "808939", "781327",
                             "795836", "788182", "799298", "799294", "799304",
                             "437197", "628865", "2761953", "2761939", "556475",
                             "666089", "2761944", "2761942", "2773149", "2773148",
                             "2773150"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), queueTracksCount);
    for (size_t i = 0; i < queueTracksCount; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i));
        mq.ChangeToNextTrack();
    }

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
}

Y_UNIT_TEST(PositiveWithOffset) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    constexpr size_t trackOffsetIndex = 3;
    constexpr size_t totalTracksCount = 10;
    constexpr size_t queueTracksCount = 7;

    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetTrackOffsetIndex(trackOffsetIndex);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "STRENGTH IN NUMB333RS");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), totalTracksCount);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), queueTracksCount);
    TStringBuf trackIds[] = {"48737126", "48737127",
                             "48737128", "48737129", "48737130", "48737131", "48737132"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), queueTracksCount);
    for (size_t i = 0; i < queueTracksCount; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i + trackOffsetIndex));
        mq.ChangeToNextTrack();
    }

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
}

Y_UNIT_TEST(PositiveWithOffsetAndHistory) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    mCtx.SetFirstRequestPageSize(20);
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    TMusicArguments::TPlaybackOptions playbackOptions;
    const size_t offset = 3;
    playbackOptions.SetTrackOffsetIndex(offset);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    const size_t queueTracksCount = 7;
    const size_t totalTracksCount = 10;

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "STRENGTH IN NUMB333RS");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), totalTracksCount);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), queueTracksCount);
    TStringBuf trackIds[] = {"48737124", "44939093", "48737125"};
    for (i32 i = offset - 1; i >= 0; i--) {
        mq.ChangeToPrevTrack();
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), trackIds[i]);
    }

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
}

Y_UNIT_TEST(PositiveWithOffsetOutOfBounds) {
    auto data = ReadTestData(POSITIVE_FILENAME);
    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig({.PageSize=31});

    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetTrackOffsetIndex(300);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "STRENGTH IN NUMB333RS");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), 10);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), 1); // QueueSize() adds +1 if you have non-empty History

    // XXX(sparkle): do we need this test? it isn't real
}

Y_UNIT_TEST(PositiveBig1WithOffset) {
    auto data = ReadTestData(POSITIVE_BIG_FILENAME);
    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig({.PageSize=31});

    constexpr size_t trackOffsetIndex = 3;
    constexpr size_t totalTracksCount = 31;
    constexpr size_t queueTracksCount = 28;

    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetTrackOffsetIndex(trackOffsetIndex);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "The Best Of 25 Years");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), totalTracksCount);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), queueTracksCount);
    TStringBuf trackIds[] = {"2761943", "545860",
                             "788179", "781328", "2761946", "788183", "788177",
                             "2761948", "2761945", "808925", "808939", "781327",
                             "795836", "788182", "799298", "799294", "799304",
                             "437197", "628865", "2761953", "2761939", "556475",
                             "666089", "2761944", "2761942", "2773149", "2773148",
                             "2773150"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), queueTracksCount);
    for (size_t i = 0; i < queueTracksCount; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i + trackOffsetIndex));
        mq.ChangeToNextTrack();
    }

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
}

Y_UNIT_TEST(PositiveBig2WithOffset2) {
    auto data = ReadTestData(POSITIVE_BIG_FILENAME);
    TMusicContext mCtx;
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig({.PageSize=31});

    constexpr size_t trackOffsetIndex = 23;
    constexpr size_t totalTracksCount = 31;
    constexpr size_t queueTracksCount = 8;

    TMusicArguments::TPlaybackOptions playbackOptions;
    playbackOptions.SetTrackOffsetIndex(trackOffsetIndex);
    mq.InitPlayback({}, rng, playbackOptions);
    ParseAlbum(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    UNIT_ASSERT_EQUAL(mq.ContentInfo().GetTitle(), "The Best Of 25 Years");
    UNIT_ASSERT_EQUAL(mq.TotalTracks(), totalTracksCount);
    UNIT_ASSERT_EQUAL(mq.QueueSize(), queueTracksCount);
    TStringBuf trackIds[] = {"2761939", "556475", "666089", "2761944",
                             "2761942", "2773149", "2773148", "2773150"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), queueTracksCount);
    for (size_t i = 0; i < queueTracksCount; ++i) {
        const auto& item = mq.CurrentItem();
        UNIT_ASSERT_EQUAL(item.GetTrackId(), trackIds[i]);
        UNIT_ASSERT_EQUAL(item.GetTrackInfo().GetPosition(), static_cast<int>(i + trackOffsetIndex));
        mq.ChangeToNextTrack();
    }

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
}

TMusicQueueWrapper loadContent(TMusicContext& mCtx, const TStringBuf albumContentJson) {
    auto& scState = *mCtx.MutableScenarioState();
    TRng rng;

    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *scState.MutableQueue());
    mq.SetConfig({.PageSize=31});
    mq.SetFiltrationMode(NScenarios::TUserPreferences_EFiltrationMode_Safe);

    TMusicArguments::TPlaybackOptions playbackOptions;
    mq.InitPlayback({}, rng, playbackOptions);

    auto data = ReadTestData(albumContentJson);
    ParseAlbum(JsonFromString(data), mq, mCtx);

    mq.ChangeState();

    return mq;
}

Y_UNIT_TEST(AlbumChildContent) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, ALBUM_CHILD_CONTENT);

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 38, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(AlbumChildContentInRoot) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, ALBUM_CHILD_CONTENT_IN_ROOT);

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "«Тайна Коко»", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 38, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(AlbumChildContentInTracks) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, ALBUM_CHILD_CONTENT_IN_TRACKS);

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "«Тайна Коко»", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 38, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(AlbumIsSuitableForChildren) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, ALBUM_IS_SUITABLE_FOR_CHILDREN);

    UNIT_ASSERT(!mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "«Тайна Коко»", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 38, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 3, TStringBuilder() << " but it was " << mq.QueueSize());
}

Y_UNIT_TEST(AlbumNeutralContent) {
    TMusicContext mCtx;
    TMusicQueueWrapper mq = loadContent(mCtx, ALBUM_NEUTRAL_CONTENT);

    UNIT_ASSERT(mq.HaveNonChildSafeContent());
    UNIT_ASSERT(!mq.HaveExplicitContent());

    UNIT_ASSERT_EQUAL_C(mq.ContentInfo().GetTitle(), "«Тайна Коко»", TStringBuilder() << " but it was " << mq.ContentInfo().GetTitle());
    UNIT_ASSERT_EQUAL_C(mq.TotalTracks(), 38, TStringBuilder() << " but it was " << mq.TotalTracks());
    UNIT_ASSERT_EQUAL_C(mq.QueueSize(), 0, TStringBuilder() << " but it was " << mq.QueueSize());
}

}

} //namespace NAlice::NHollywood::NMusic
