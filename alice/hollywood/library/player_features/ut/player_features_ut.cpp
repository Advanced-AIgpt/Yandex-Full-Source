#include <alice/hollywood/library/player_features/player_features.h>

#include <alice/hollywood/library/request/request.h>

#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/megamind/protos/common/device_state.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {


Y_UNIT_TEST_SUITE(CalcPlayerFeaturesTest) {

TScenarioRunRequest MakeRunRequestWithMusicPlayer() {
    TScenarioRunRequest requestProto;
    requestProto.MutableBaseRequest()->SetServerTimeMs(1600000065000);
    requestProto.MutableBaseRequest()->MutableClientInfo()->SetEpoch("1600000060");

    TDeviceState deviceState;
    JsonToProto(JsonFromString(R"-({
        "music": {
            "player": {
                "pause": true
            },
            "currently_playing": {
                "track_id": "1234567890"
            },
            "last_play_timestamp": 1600000000000
        },
    })-"), deviceState);
    *requestProto.MutableBaseRequest()->MutableDeviceState() = deviceState;
    return requestProto;
}

Y_UNIT_TEST(ForOwnerOfMusicPlayer) {
    const auto requestProto = MakeRunRequestWithMusicPlayer();
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    auto actual = CalcPlayerFeaturesForMusicPlayer(TRTLogger::NullLogger(), request, [](const TDeviceState&) { return true; });

    TScenarioRunResponse_TFeatures_TPlayerFeatures expected;
    expected.SetRestorePlayer(true);
    expected.SetSecondsSincePause(60);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
}

Y_UNIT_TEST(ForNonOwnerOfMusicPlayer) {
    const auto requestProto = MakeRunRequestWithMusicPlayer();
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    auto actual = CalcPlayerFeaturesForMusicPlayer(TRTLogger::NullLogger(), request, [](const TDeviceState&) { return false; });

    TScenarioRunResponse_TFeatures_TPlayerFeatures expected;
    expected.SetRestorePlayer(false);
    expected.SetSecondsSincePause(0);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
}


TScenarioRunRequest MakeRunRequestWithAudioPlayer() {
    TScenarioRunRequest requestProto;
    requestProto.MutableBaseRequest()->SetServerTimeMs(1600000065000);
    requestProto.MutableBaseRequest()->MutableClientInfo()->SetEpoch("1600000060");

    TDeviceState deviceState;
    JsonToProto(JsonFromString(R"-({
        "audio_player": {
            "player_state": "Stopped",
            "offset_ms": 9000,
            "current_stream": {
                "stream_id": "qwerty123",
                "last_play_timestamp": 1600000010000
            },
            "last_play_timestamp": 1600000000000,
            "duration_ms": 216000
        }
    })-"), deviceState);
    *requestProto.MutableBaseRequest()->MutableDeviceState() = deviceState;
    return requestProto;
}

Y_UNIT_TEST(ForOwnerOfAudioPlayer) {
    const auto requestProto = MakeRunRequestWithAudioPlayer();
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    auto actual = CalcPlayerFeaturesForAudioPlayer(TRTLogger::NullLogger(), request, [](const TDeviceState&) { return true; });

    TScenarioRunResponse_TFeatures_TPlayerFeatures expected;
    expected.SetRestorePlayer(true);
    expected.SetSecondsSincePause(60);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
}

Y_UNIT_TEST(ForNonOwnerOfAudioPlayer) {
    const auto requestProto = MakeRunRequestWithAudioPlayer();
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    auto actual = CalcPlayerFeaturesForAudioPlayer(TRTLogger::NullLogger(), request, [](const TDeviceState&) { return false; });

    TScenarioRunResponse_TFeatures_TPlayerFeatures expected;
    expected.SetRestorePlayer(false);
    expected.SetSecondsSincePause(0);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
}


TScenarioRunRequest MakeRunRequestWithMusicPlayerAndNegativeSecondsSinceLastPlay() {
    TScenarioRunRequest requestProto;
    requestProto.MutableBaseRequest()->SetServerTimeMs(1500000065000);
    requestProto.MutableBaseRequest()->MutableClientInfo()->SetEpoch("1500000060");

    TDeviceState deviceState;
    JsonToProto(JsonFromString(R"-({
        "music": {
            "player": {
                "pause": true
            },
            "currently_playing": {
                "track_id": "1234567890"
            },
            "last_play_timestamp": 1600000000000
        },
    })-"), deviceState);
    *requestProto.MutableBaseRequest()->MutableDeviceState() = deviceState;
    return requestProto;
}

Y_UNIT_TEST(ForOwnerOfMusicPlayerNegativeSecondsSinceLastPlay) {
    const auto requestProto = MakeRunRequestWithMusicPlayerAndNegativeSecondsSinceLastPlay();
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    auto actual = CalcPlayerFeaturesForMusicPlayer(TRTLogger::NullLogger(), request, [](const TDeviceState&) { return true; });

    TScenarioRunResponse_TFeatures_TPlayerFeatures expected;
    expected.SetRestorePlayer(true);
    expected.SetSecondsSincePause(0);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
}

TScenarioRunRequest MakeRunRequestWithMusicPlayerAndSecondsMoreThanAWeek() {
    TScenarioRunRequest requestProto;
    requestProto.MutableBaseRequest()->SetServerTimeMs(1700000065000);
    requestProto.MutableBaseRequest()->MutableClientInfo()->SetEpoch("1700000060");

    TDeviceState deviceState;
    JsonToProto(JsonFromString(R"-({
        "music": {
            "player": {
                "pause": true
            },
            "currently_playing": {
                "track_id": "1234567890"
            },
            "last_play_timestamp": 1600000000000
        },
    })-"), deviceState);
    *requestProto.MutableBaseRequest()->MutableDeviceState() = deviceState;
    return requestProto;
}

Y_UNIT_TEST(ForOwnerOfMusicPlayerSecondsMoreThanAWeek) {
    const auto requestProto = MakeRunRequestWithMusicPlayerAndSecondsMoreThanAWeek();
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};

    auto actual = CalcPlayerFeaturesForMusicPlayer(TRTLogger::NullLogger(), request, [](const TDeviceState&) { return true; });

    TScenarioRunResponse_TFeatures_TPlayerFeatures expected;
    expected.SetRestorePlayer(false);
    expected.SetSecondsSincePause(0);
    UNIT_ASSERT_MESSAGES_EQUAL(expected, actual);
}

}

} // namespace NAlice::NHollywood
