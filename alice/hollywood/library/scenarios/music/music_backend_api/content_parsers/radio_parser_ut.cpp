#include "radio_parser.h"
#include "artist_parser.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/music_config/music_config.h>
#include <alice/library/json/json.h>
#include <alice/library/unittest/mock_sensors.h>
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
constexpr TStringBuf RADIO_FILENAME = "radio.json";
constexpr TStringBuf RADIO_NEW_SESSION_FILENAME = "radio_new_session.json";
constexpr TStringBuf RADIO_SESSION_TRACKS_FILENAME = "radio_session_tracks.json";
constexpr TStringBuf RADIO_REPEATS_FILENAME = "radio_repeats.json";
constexpr TStringBuf RADIO_PUMPKIN_FILENAME = "radio_pumpkin.json";
constexpr TStringBuf ARTIST_FILENAME = "artist-positive.json";

TString ReadTestData(TStringBuf filename) {
    return TFileInput(TFsPath(ArcadiaSourceRoot()) / TEST_DATA_DIR / filename).ReadAll();
}

} // namespace

Y_UNIT_TEST_SUITE(RadioParserTest) {

Y_UNIT_TEST(Smoke) {
    auto data = ReadTestData(RADIO_FILENAME);
    TMusicContext mCtx;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    mq.InitPlayback({}, rng);
    auto sensors = TNoopSensors();
    ParseRadio(TRTLogger::NullLogger(), sensors, JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    const size_t tracksCount = 5;

    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
    TStringBuf trackIds[] = {"27373919", "628432", "202843", "19313871", "35972"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), tracksCount);
    for (size_t i = 0; i < tracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), trackIds[i]);
        mq.ChangeToNextTrack();
    }
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioBatchId(), "c8277792-0d2c-57d5-b465-a3dabbadoo00");
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioSessionId(), "foobarbaz");
}

Y_UNIT_TEST(NewSessionSmoke) {
    auto data = ReadTestData(RADIO_NEW_SESSION_FILENAME);
    TMusicContext mCtx;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    mq.InitPlayback({}, rng);
    auto sensors = TNoopSensors();
    ParseRadio(TRTLogger::NullLogger(), sensors, JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    const size_t tracksCount = 5;

    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
    TStringBuf trackIds[] = {"10459623", "70066658", "786859", "43375039", "48039052"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), tracksCount);
    for (size_t i = 0; i < tracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), trackIds[i]);
        mq.ChangeToNextTrack();
    }
    UNIT_ASSERT_STRINGS_EQUAL(mq.ContentId().GetId(), "genre:rock");
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioBatchId(), "47fd7720-b125-480d-a265-6034f434758b");
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioSessionId(), "foobarbaz");
}

Y_UNIT_TEST(SessionTracksSmoke) {
    auto data = ReadTestData(RADIO_SESSION_TRACKS_FILENAME);
    TMusicContext mCtx;
    TRng rng;

    auto& queue = *mCtx.MutableScenarioState()->MutableQueue();
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), queue);
    mq.SetConfig(CreateMusicConfig({}));

    queue.MutableCurrentContentLoadingState()->MutableRadio()->SetSessionId("foobarbaz"); // scenario already knows its radioSessionId
    queue.MutableCurrentContentLoadingState()->MutableRadio()->SetBatchId("a1a1c6f6-fa50-441f-b0f6-087fc38649d0"); // scenario already knows its batchId

    mq.InitPlayback({}, rng);
    auto sensors = TNoopSensors();
    ParseRadio(TRTLogger::NullLogger(), sensors, JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    const size_t tracksCount = 5;

    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
    TStringBuf trackIds[] = {"43375039", "10459623", "70066658", "21044637", "68821184"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), tracksCount);
    for (size_t i = 0; i < tracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), trackIds[i]);
        mq.ChangeToNextTrack();
    }

    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioBatchId(), "a1a1c6f6-fa50-441f-b0f6-087fc38649d0"); // batch id unchanged
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioSessionId(), "foobarbaz"); // session id unchanged
}

Y_UNIT_TEST(RepeatedTracksSensors) {
    TMusicContext mCtx;
    TRng rng;

    auto& queue = *mCtx.MutableScenarioState()->MutableQueue();
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), queue);

    mq.SetConfig(CreateMusicConfig({}));

    queue.MutableCurrentContentLoadingState()->MutableRadio()->SetSessionId("foobarbaz"); // scenario already knows its radioSessionId
    queue.MutableCurrentContentLoadingState()->MutableRadio()->SetBatchId("a1a1c6f6-fa50-441f-b0f6-087fc38649d0"); // scenario already knows its batchId
    *queue.MutableNextContentLoadingState() = queue.GetCurrentContentLoadingState();

    auto sensors = TFakeSensors();

    // ASK FOR RADIO
    TContentId radioContentId;
    radioContentId.AddIds("language:italian");
    radioContentId.SetType(TContentId_EContentType_Radio);
    mq.SetContentId(radioContentId);

    auto data = ReadTestData(RADIO_SESSION_TRACKS_FILENAME);
    ParseRadio(TRTLogger::NullLogger(), sensors, JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    const size_t firstTracksCount = 5;
    const TStringBuf firstTrackIds[firstTracksCount] = {"43375039", "10459623", "70066658", "21044637", "68821184"};
    for (size_t i = 0; i < firstTracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), firstTrackIds[i]);
        const auto neededStatus = i + 1 == firstTracksCount ? ETrackChangeResult::NeedStateUpdate : ETrackChangeResult::TrackChanged;
        UNIT_ASSERT_EQUAL(mq.ChangeToNextTrack(), neededStatus);
    }

    // ASK FOR ON-DEMAND
    TContentId artistContentId;
    artistContentId.SetId("artist:012345");
    artistContentId.SetType(TContentId_EContentType_Artist);
    mq.SetContentId(artistContentId);

    data = ReadTestData(ARTIST_FILENAME);
    ParseArtist(JsonFromString(data), mq, mCtx);
    mq.ChangeState();

    const size_t secondTracksCount = 3;
    const TStringBuf secondTrackIds[secondTracksCount] = {"133207", "133054", "216187"};
    for (size_t i = 0; i < secondTracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), secondTrackIds[i]);
        UNIT_ASSERT_EQUAL(mq.ChangeToNextTrack(), ETrackChangeResult::TrackChanged);
    }

    // ASK FOR RADIO WITH REPEATS
    queue.MutableCurrentContentLoadingState()->MutableRadio()->SetSessionId("foobarbaz"); // scenario already knows its radioSessionId
    queue.MutableCurrentContentLoadingState()->MutableRadio()->SetBatchId("a1a1c6f6-fa50-441f-b0f6-087fc38649d0"); // scenario already knows its batchId
    *queue.MutableNextContentLoadingState() = queue.GetCurrentContentLoadingState();

    TContentId repeatedRadioContentId;
    repeatedRadioContentId.AddIds("mood:happy");
    repeatedRadioContentId.SetType(TContentId_EContentType_Radio);
    mq.SetContentId(repeatedRadioContentId);

    data = ReadTestData(RADIO_REPEATS_FILENAME);
    ParseRadio(TRTLogger::NullLogger(), sensors, JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    const size_t thirdTracksCount = 6;
    const TStringBuf thirdTrackIds[thirdTracksCount] = {
        "43375039", // sensor +1, has been at first radio
        "1234567", // nothing, random id
        "133054", // nothing, has been at artist on-demand
        "21044637", // sensor +1, has been at first radio
        "216187", // nothing, has been at artist on-demand
        "70066658", // sensor +1, has been at first radio
    };
    for (size_t i = 0; i < thirdTracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), thirdTrackIds[i]);
        const auto neededStatus = i + 1 == thirdTracksCount ? ETrackChangeResult::NeedStateUpdate : ETrackChangeResult::TrackChanged;
        UNIT_ASSERT_EQUAL(mq.ChangeToNextTrack(), neededStatus);
    }

    // CHECK SENSORS
    const TFakeSensors::TRateSensor* repeatedSensor = sensors.FindFirstRateSensor("name", "thin_client_radio_repeated_tracks");
    UNIT_ASSERT(repeatedSensor);
    UNIT_ASSERT_EQUAL(repeatedSensor->Value, 3);

    const TFakeSensors::TRateSensor* pumpkinSensor = sensors.FindFirstRateSensor("name", "thin_client_radio_pumpkins");
    UNIT_ASSERT(!pumpkinSensor);
}

Y_UNIT_TEST(PumpkinSensors) {
    auto data = ReadTestData(RADIO_PUMPKIN_FILENAME);
    TMusicContext mCtx;
    TRng rng;
    TMusicQueueWrapper mq(TRTLogger::NullLogger(), *mCtx.MutableScenarioState()->MutableQueue());
    mq.SetConfig(CreateMusicConfig({}));

    mq.InitPlayback({}, rng);
    auto sensors = TFakeSensors();
    ParseRadio(TRTLogger::NullLogger(), sensors, JsonFromString(data), mq, /* hasMusicSubscription = */ true);
    mq.ChangeState();

    const size_t tracksCount = 5;

    UNIT_ASSERT_EQUAL(mq.QueueSize(), tracksCount);
    TStringBuf trackIds[] = {"3891036", "150007", "34430896", "13115262", "368215"};
    UNIT_ASSERT_EQUAL(std::distance(std::begin(trackIds), std::end(trackIds)), tracksCount);
    for (size_t i = 0; i < tracksCount; ++i) {
        UNIT_ASSERT_EQUAL(mq.CurrentItem().GetTrackId(), trackIds[i]);
        mq.ChangeToNextTrack();
    }
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioBatchId(), "a8b6b797-cddf-4054-9d05-ab0765061a2b");
    UNIT_ASSERT_STRINGS_EQUAL(mq.GetRadioSessionId(), "<PUMPKIN>");

    const TFakeSensors::TRateSensor* pumpkinSensor = sensors.FindFirstRateSensor("name", "thin_client_radio_pumpkins");
    UNIT_ASSERT(pumpkinSensor);
    UNIT_ASSERT_EQUAL(pumpkinSensor->Value, 1);
}

}

} //namespace NAlice::NHollywood::NMusic
