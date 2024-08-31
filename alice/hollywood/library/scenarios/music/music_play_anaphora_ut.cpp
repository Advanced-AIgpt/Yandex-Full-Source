#include "common.h"
#include "music_play_anaphora.h"

#include <alice/library/experiments/flags.h>
#include <alice/library/json/json.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const int ALBUM_ID = 8432683;
const int ARTIST_ID = 1438;
const TString TRACK_ID = "81561";

TProtoStruct CreateTrackInfo() {
    NJson::TJsonValue info;
    info["id"] = TRACK_ID;

    NJson::TJsonValue album;
    album["id"] = ALBUM_ID;
    info["albums"].AppendValue(album);

    NJson::TJsonValue artist;
    artist["id"] = ARTIST_ID;
    info["artists"].AppendValue(artist);

    return JsonToProto<TProtoStruct>(info);
}

NScenarios::TScenarioRunRequest PrepareRequestProto(const bool putExp, const bool putMusic) {
    NScenarios::TScenarioRunRequest proto;

    if (putExp) {
        auto& experiments = *proto.MutableBaseRequest()->MutableExperiments()->mutable_fields();
        experiments[TString{NExperiments::EXP_HOLLYWOOD_MUSIC_PLAY_ANAPHORA}].set_string_value("1");
    }

    if (putMusic) {
        auto& deviceState = *proto.MutableBaseRequest()->MutableDeviceState();
        auto& trackInfo = *deviceState.MutableMusic()->MutableCurrentlyPlaying()->MutableRawTrackInfo();

        trackInfo = CreateTrackInfo();
    }

    return proto;
}

} // namespace

Y_UNIT_TEST_SUITE(CheckAndGetMusicPlayAnaphoraTrack) {
    Y_UNIT_TEST(NoFrame) {
        const auto proto = PrepareRequestProto(/* putExp= */ true, /* putMusic= */ true);
        NAppHost::NService::TTestContext serviceCtx;
        TScenarioRunRequestWrapper request(proto, serviceCtx);

        UNIT_ASSERT(nullptr == CheckAndGetMusicPlayAnaphoraTrack(nullptr, request));
    }

    Y_UNIT_TEST(FramePresent) {
        TFrame frame{""};

        for (const bool putExp : {false, true}) {
            for (const bool putMusic : {false, true}) {
                const bool shouldReturnTrackInfo = (putExp && putMusic);
                const auto proto = PrepareRequestProto(putExp, putMusic);
                NAppHost::NService::TTestContext serviceCtx;
                TScenarioRunRequestWrapper request(proto, serviceCtx);

                const auto* trackInfo = CheckAndGetMusicPlayAnaphoraTrack(&frame, request);
                if (shouldReturnTrackInfo) {
                    const auto& deviceState = proto.GetBaseRequest().GetDeviceState();
                    UNIT_ASSERT(trackInfo == &deviceState.GetMusic().GetCurrentlyPlaying().GetRawTrackInfo());
                } else {
                    UNIT_ASSERT(trackInfo == nullptr);
                }
            }
        }
    }
}

Y_UNIT_TEST_SUITE(TransformMusicPlayAnaphora) {
    Y_UNIT_TEST(CreatesMusicPlayFrame) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        UNIT_ASSERT_VALUES_EQUAL(MUSIC_PLAY_FRAME, result.Name());
    }

    Y_UNIT_TEST(CopiesSlots) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};
        frame.AddSlot({"foo", "foo_type", TSlot::TValue{"foo_value"}});

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto fooSlot = result.FindSlot("foo");
        UNIT_ASSERT(fooSlot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL("foo_value", fooSlot->Value.AsString());
    }

    Y_UNIT_TEST(DoesntCopyTargetType) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};
        frame.AddSlot({"target_type", "custom.target_type", TSlot::TValue{"album"}});

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot("target_type");
        UNIT_ASSERT(slot == nullptr);
    }

    Y_UNIT_TEST(SetsOriginalIntent) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot(ORIGINAL_INTENT);
        UNIT_ASSERT(slot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(MUSIC_PLAY_ANAPHORA_FRAME, slot->Value.AsString());
    }

    Y_UNIT_TEST(Album) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};
        frame.AddSlot({"target_type", "custom.target_type", TSlot::TValue{"album"}});

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot("album_id");
        UNIT_ASSERT(slot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(ToString(ALBUM_ID), slot->Value.AsString());

        UNIT_ASSERT(result.FindSlot("artist_id") == nullptr);
        UNIT_ASSERT(result.FindSlot("track_id") == nullptr);
    }

    Y_UNIT_TEST(Artist) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};
        frame.AddSlot({"target_type", "custom.target_type", TSlot::TValue{"artist"}});

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot("artist_id");
        UNIT_ASSERT(slot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(ToString(ARTIST_ID), slot->Value.AsString());

        UNIT_ASSERT(result.FindSlot("album_id") == nullptr);
        UNIT_ASSERT(result.FindSlot("track_id") == nullptr);
    }

    Y_UNIT_TEST(ArtistByDefault) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot("artist_id");
        UNIT_ASSERT(slot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(ToString(ARTIST_ID), slot->Value.AsString());

        UNIT_ASSERT(result.FindSlot("album_id") == nullptr);
        UNIT_ASSERT(result.FindSlot("track_id") == nullptr);
    }

    Y_UNIT_TEST(Track) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};
        frame.AddSlot({"target_type", "custom.target_type", TSlot::TValue{"track"}});

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot("track_id");
        UNIT_ASSERT(slot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(TRACK_ID, slot->Value.AsString());

        UNIT_ASSERT(result.FindSlot("artist_id") == nullptr);
        UNIT_ASSERT(result.FindSlot("album_id") == nullptr);
    }

    Y_UNIT_TEST(NeedSimilarDefault) {
        const auto trackInfo = CreateTrackInfo();
        TFrame frame{MUSIC_PLAY_ANAPHORA_FRAME};
        frame.AddSlot({"need_similar", "custom.need_similar", TSlot::TValue{"need_similar"}});

        const auto result = TransformMusicPlayAnaphora(frame, trackInfo);
        const auto slot = result.FindSlot("track_id");
        UNIT_ASSERT(slot != nullptr);
        UNIT_ASSERT_VALUES_EQUAL(TRACK_ID, slot->Value.AsString());

        UNIT_ASSERT(result.FindSlot("artist_id") == nullptr);
        UNIT_ASSERT(result.FindSlot("album_id") == nullptr);
    }
}

} // namespace NAlice::NHollywood::NMusic
