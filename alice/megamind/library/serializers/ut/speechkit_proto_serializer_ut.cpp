#include "shared_resources.h"
#include "shared_ut.h"

#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/serialized_enum.h>

namespace NAlice::NMegamind {

#if defined(ASSERT_SERIALIZE_PROTO_MODEL)
#error ASSERT_SERIALIZE_PROTO_MODEL is already defined!
#endif

#define ASSERT_SERIALIZE_PROTO_MODEL(Type, model, skJsonStringBuf)                                                    \
    do {                                                                                                              \
        Type expectedProto;                                                                                           \
        google::protobuf::util::JsonParseOptions options;                                                             \
        options.ignore_unknown_fields = false;                                                                        \
        UNIT_ASSERT(                                                                                                  \
            google::protobuf::util::JsonStringToMessage(TString{skJsonStringBuf}, &expectedProto, options).ok());     \
        const auto actualProto = GetSpeechKitProtoModelSerializer().Serialize(model);                                 \
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);                                                       \
    } while (false)

Y_UNIT_TEST_SUITE_F(TestTSpeechKitProtoSerializer, TFixture) {
    Y_UNIT_TEST(TestSerializeTActionButtonModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TButton, GetActionButtonModel(), ACTION_BUTTON);
    }

    Y_UNIT_TEST(TestSerializeTThemedActionButtonModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TButton, GetThemedActionButtonModel(),
                                     THEMED_ACTION_BUTTON);
    }

    Y_UNIT_TEST(TestSerializeTDiv2CardModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, GetDiv2CardModel(), DIV2_CARD);
    }

    Y_UNIT_TEST(TestSerializeTDivCardModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, GetDivCardModel(), DIV_CARD);
    }

    Y_UNIT_TEST(TestSerializeTTextCardModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, GetTextCardModel(), TEXT_CARD);
    }

    Y_UNIT_TEST(TestSerializeTTextWithButtonCardModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(TSpeechKitResponseProto_TResponse_TCard, GetTextWithButtonCardModel(),
                                     TEXT_WITH_BUTTON);
    }

    Y_UNIT_TEST(TestSerilizeTAlarmNewDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetAlarmNewDirectiveModel(), ALARM_NEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerilizeTAlarmSetSoundDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetAlarmSetSoundDirectiveModel(),
                                     ALARM_SET_SOUND_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCarDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetCarDirectiveModel(), CAR_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCloseDialogDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetCloseDialogDirectiveModel(), CLOSE_DIALOG_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCloseDialogDirectiveModelWithScreenId) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetCloseDialogDirectiveModelWithScreenId(),
                                     CLOSE_DIALOG_DIRECTIVE_WITH_SCREEN_ID);
    }

    Y_UNIT_TEST(TestSerializeTEndDialogSessionDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetEndDialogSessionDirectiveModel(),
                                     END_DIALOG_SESSION_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTFindContactsDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetFindContactsDirectiveModel(), FIND_CONTACTS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetMusicPlayDirectiveModel(), MUSIC_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModelWithEndpointId) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetMusicPlayDirectiveModelWithEndpointId(),
                                     MUSIC_PLAY_DIRECTIVE_WITH_ENDPOINT_ID);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModel2) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetMusicPlayDirectiveModel2(), MUSIC_PLAY_DIRECTIVE_2);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModelEmptyRoom) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetMusicPlayDirectiveModelEmptyRoom(), MUSIC_PLAY_DIRECTIVE_EMPTY_ROOM);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModelWithLocationInfo) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetMusicPlayDirectiveModelWithLocationInfo(), MUSIC_PLAY_DIRECTIVE_WITH_LOCATION_INFO);
    }

    Y_UNIT_TEST(TestSerializeTOpenDialogDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetOpenDialogDirectiveModel(), OPEN_DIALOG_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTOpenSettingsDirectiveModel) {
        for (const auto target : GetEnumAllValues<ESettingsTarget>()) {
            ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetOpenSettingsDirectiveModel(target),
                                         GetOpenSettingsSkJsonString(target));
        }
    }

    Y_UNIT_TEST(TestSerializeTOpenUriDirective) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetOpenUriDirectiveModel(), OPEN_URI_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTOpenUriDirectiveWithScreenId) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetOpenUriDirectiveModelWithScreenId(), OPEN_URI_DIRECTIVE_WITH_SCREEN_ID);
    }

    Y_UNIT_TEST(TestSerializeTPlayerRewindDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective,
                                     GetPlayerRewindDirectiveModel(EPlayerRewindType::Absolute),
                                     PLAYER_REWIND_DIRECTIVE_ABSOLUTE);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetPlayerRewindDirectiveModel(EPlayerRewindType::Forward),
                                     PLAYER_REWIND_DIRECTIVE_FORWARD);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective,
                                     GetPlayerRewindDirectiveModel(EPlayerRewindType::Backward),
                                     PLAYER_REWIND_DIRECTIVE_BACKWARD);
    }

    Y_UNIT_TEST(TestSerializeTSetCookiesDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetSetCookiesDirectiveModel(), SET_COOKIES_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTSetSearchFilterDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective,
                                     GetSetSearchFilterDirectiveModel(ESearchFilterLevel::None),
                                     SET_SEARCH_FILTER_DIRECTIVE_NONE);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective,
                                     GetSetSearchFilterDirectiveModel(ESearchFilterLevel::Strict),
                                     SET_SEARCH_FILTER_DIRECTIVE_STRICT);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective,
                                     GetSetSearchFilterDirectiveModel(ESearchFilterLevel::Moderate),
                                     SET_SEARCH_FILTER_DIRECTIVE_MODERATE);
    }

    Y_UNIT_TEST(TestSerializeTSetTimerDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetSetTimerDirectiveModel(/* setTimestamp= */ false),
                                     SET_TIMER_DIRECTIVE);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetSetTimerDirectiveModel(/* setTimestamp= */ true),
                                     SET_TIMER_TIMESTAMP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowGalleryDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetShowGalleryDirectiveModel(), SHOW_GALLERY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowPayPushScreenDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetShowPayPushScreenDirectiveModel(),
                                     SHOW_PAY_PUSH_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowSeasonGalleryDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetShowSeasonGalleryDirectiveModel(),
                                     SHOW_SEASON_GALLERY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowVideoDescriptionDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetShowVideoDescriptionDirectiveModel(),
                                     SHOW_VIDEO_DESCRIPTION_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTSoundSetLevelDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetSoundSetLevelDirectiveModel(),
                                     SOUND_SET_LEVEL_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTSoundSetLevelInLocationDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetSoundSetLevelInLocationDirectiveModel(),
                                     SOUND_SET_LEVEL_DIRECTIVE_IN_LOCATION);
    }

    Y_UNIT_TEST(TestSerializeTSoundSetLevelDirectiveModelWithMultiroom) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetSoundSetLevelDirectiveModelWithMultiroom(),
                                     SOUND_SET_LEVEL_DIRECTIVE_WITH_MULTIROOM);
    }

    Y_UNIT_TEST(TestSerializeTStartMultiroomDirectiveWithLocationInfoAndIncludeCurrentDeviceSetTrue) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetStartMultiroomDirectiveModelWithLocationInfoAndIncludeCurrentDevice(),
                                     START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO_2);
    }

    Y_UNIT_TEST(TestSerializeTThereminPlayDirectiveModelWithExternalSetModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetThereminPlayDirectiveModelWithExternalSet(),
                                     THEREMIN_PLAY_DIRECTIVE_WITH_EXTERNAL_SET);
    }

    Y_UNIT_TEST(TestSerializeTThereminPlayDirectiveModelWithInternalSetModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetThereminPlayDirectiveModelWithInternalSet(),
                                     THEREMIN_PLAY_DIRECTIVE_WITH_INTERNAL_SET);
    }

    Y_UNIT_TEST(TestSerializeTTypeTextDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetTypeTextDirectiveModel(), TYPE_TEXT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTTypeTextSilentDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetTypeTextSilentDirectiveModel(),
                                     TYPE_TEXT_SILENT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTUpdateDialogInfoDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetUpdateDialogInfoDirectiveModel(),
                                     UPDATE_DIALOG_INFO_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTVideoPlayDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetVideoPlayDirectiveModel(), VIDEO_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTYandexNaviDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetYandexNaviDirectiveModel(), YANDEXNAVI_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCallbackDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetCallbackDirectiveModel(), CALLBACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCallbackDirectiveModelWithMultiroom) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetCallbackDirectiveModelWithMultiroom(), CALLBACK_DIRECTIVE_WITH_MULTIROOM);
    }

    Y_UNIT_TEST(TestSerializeTGetNextCallbackDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetGetNextCallbackDirectiveModel(), GET_NEXT_CALLBACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAddContactBookAsrDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetAddContactBookAsrDirectiveModel(),
                                     ADD_CONTACT_BOOK_ASR_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTDeferApplyDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetDeferApplyDirectiveModel(), DEFER_APPLY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTGoBackwardDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetVisualGoBackwardDirectiveModel(), VISUAL_GO_BACK_DIRECTIVE);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetHistoricalGoBackwardDirectiveModel(), HISTORICAL_GO_BACK_DIRECTIVE);
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetNativeGoBackwardDirectiveModel(), NATIVE_GO_BACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAudioPlayDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetAudioPlayDirectiveModel(), AUDIO_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowButtonsDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetShowButtonsDirectiveModel(), SHOW_BUTTONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAddConditionalActionsDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetAddConditionalActionsDirectiveModel(), ADD_CONDITIONAL_ACTIONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAddExternalEntitiesDescriptionDirectiveModel) {
        ASSERT_SERIALIZE_PROTO_MODEL(NSpeechKit::TDirective, GetAddExternalEntitiesDescriptionDirectiveModel(), ADD_EXTERNAL_ENTITIES_DESCRIPTION_DIRECTIVE);
    }
}

} // namespace NAlice::NMegamind
