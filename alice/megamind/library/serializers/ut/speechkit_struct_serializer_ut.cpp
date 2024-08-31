#include "shared_resources.h"
#include "shared_ut.h"

#include <alice/library/unittest/message_diff.h>

#include <google/protobuf/util/json_util.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/serialized_enum.h>

namespace NAlice::NMegamind {

#if defined(ASSERT_SERIALIZE_STRUCT_MODEL)
#error ASSERT_SERIALIZE_STRUCT_MODEL is already defined!
#endif

#define ASSERT_SERIALIZE_STRUCT_MODEL(model, skJsonStringBuf)                                                         \
    do {                                                                                                              \
        google::protobuf::Struct expectedProto;                                                                       \
        google::protobuf::util::JsonParseOptions options;                                                             \
        options.ignore_unknown_fields = false;                                                                        \
        const auto status =                                                                                           \
            google::protobuf::util::JsonStringToMessage(TString{skJsonStringBuf}, &expectedProto, options);         \
        UNIT_ASSERT_C(status.ok(), status.ToString());                                                                \
        const auto actualProto = GetSpeechKitStructModelSerializer().Serialize(model);                                \
        UNIT_ASSERT_MESSAGES_EQUAL(expectedProto, actualProto);                                                       \
    } while (false)

Y_UNIT_TEST_SUITE_F(TestTSpeechKitStructSerializer, TFixture) {
    Y_UNIT_TEST(TestSerializeTActionButtonModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetActionButtonModel(), ACTION_BUTTON);
    }

    Y_UNIT_TEST(TestSerializeTThemedActionButtonModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetThemedActionButtonModel(), THEMED_ACTION_BUTTON);
    }

    Y_UNIT_TEST(TestSerializeTDiv2CardModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetDiv2CardModel(), DIV2_CARD);
    }

    Y_UNIT_TEST(TestSerializeTDivCardModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetDivCardModel(), DIV_CARD);
    }

    Y_UNIT_TEST(TestSerializeTTextCardModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetTextCardModel(), TEXT_CARD);
    }

    Y_UNIT_TEST(TestSerializeTTextWithButtonCardModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetTextWithButtonCardModel(), TEXT_WITH_BUTTON);
    }

    Y_UNIT_TEST(TestSerilizeTAlarmNewDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetAlarmNewDirectiveModel(), ALARM_NEW_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerilizeTAlarmSetSoundDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetAlarmSetSoundDirectiveModel(), ALARM_SET_SOUND_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCarDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetCarDirectiveModel(), CAR_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCloseDialogDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetCloseDialogDirectiveModel(), CLOSE_DIALOG_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCloseDialogDirectiveModelWithScreenId) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetCloseDialogDirectiveModelWithScreenId(),
                                      CLOSE_DIALOG_DIRECTIVE_WITH_SCREEN_ID);
    }

    Y_UNIT_TEST(TestSerializeTEndDialogSessionDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetEndDialogSessionDirectiveModel(), END_DIALOG_SESSION_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTFindContactsDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetFindContactsDirectiveModel(), FIND_CONTACTS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetMusicPlayDirectiveModel(), MUSIC_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModel2) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetMusicPlayDirectiveModel2(), MUSIC_PLAY_DIRECTIVE_2);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModelEmptyRoom) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetMusicPlayDirectiveModelEmptyRoom(), MUSIC_PLAY_DIRECTIVE_EMPTY_ROOM);
    }

    Y_UNIT_TEST(TestSerializeTMusicPlayDirectiveModelWithLocationInfo) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetMusicPlayDirectiveModelWithLocationInfo(), MUSIC_PLAY_DIRECTIVE_WITH_LOCATION_INFO);
    }

    Y_UNIT_TEST(TestSerializeTOpenDialogDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetOpenDialogDirectiveModel(), OPEN_DIALOG_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTOpenSettingsDirectiveModel) {
        for (const auto target : GetEnumAllValues<ESettingsTarget>()) {
            ASSERT_SERIALIZE_STRUCT_MODEL(GetOpenSettingsDirectiveModel(target), GetOpenSettingsSkJsonString(target));
        }
    }

    Y_UNIT_TEST(TestSerializeTOpenUriDirective) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetOpenUriDirectiveModel(), OPEN_URI_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTOpenUriDirectiveWithScreenId) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetOpenUriDirectiveModelWithScreenId(), OPEN_URI_DIRECTIVE_WITH_SCREEN_ID);
    }

    Y_UNIT_TEST(TestSerializeTPlayerRewindDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetPlayerRewindDirectiveModel(EPlayerRewindType::Absolute),
                                      PLAYER_REWIND_DIRECTIVE_ABSOLUTE);
        ASSERT_SERIALIZE_STRUCT_MODEL(GetPlayerRewindDirectiveModel(EPlayerRewindType::Forward),
                                      PLAYER_REWIND_DIRECTIVE_FORWARD);
        ASSERT_SERIALIZE_STRUCT_MODEL(GetPlayerRewindDirectiveModel(EPlayerRewindType::Backward),
                                      PLAYER_REWIND_DIRECTIVE_BACKWARD);
    }

    Y_UNIT_TEST(TestSerializeTSetCookieDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSetCookiesDirectiveModel(), SET_COOKIES_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTSetSearchFilterDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSetSearchFilterDirectiveModel(ESearchFilterLevel::None),
                                      SET_SEARCH_FILTER_DIRECTIVE_NONE);
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSetSearchFilterDirectiveModel(ESearchFilterLevel::Strict),
                                      SET_SEARCH_FILTER_DIRECTIVE_STRICT);
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSetSearchFilterDirectiveModel(ESearchFilterLevel::Moderate),
                                      SET_SEARCH_FILTER_DIRECTIVE_MODERATE);
    }

    Y_UNIT_TEST(TestSerializeTSetTimerDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSetTimerDirectiveModel(/* setTimestamp= */ false), SET_TIMER_DIRECTIVE);
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSetTimerDirectiveModel(/* setTimestamp= */ true),
                                      SET_TIMER_TIMESTAMP_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowGalleryDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetShowGalleryDirectiveModel(), SHOW_GALLERY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowPayPushScreenDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetShowPayPushScreenDirectiveModel(), SHOW_PAY_PUSH_SCREEN_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowSeasonGalleryDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetShowSeasonGalleryDirectiveModel(), SHOW_SEASON_GALLERY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowVideoDescriptionDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetShowVideoDescriptionDirectiveModel(), SHOW_VIDEO_DESCRIPTION_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTSoundSetLevelDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSoundSetLevelDirectiveModel(), SOUND_SET_LEVEL_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTSoundSetLevelInLocationDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSoundSetLevelInLocationDirectiveModel(), SOUND_SET_LEVEL_DIRECTIVE_IN_LOCATION);
    }

    Y_UNIT_TEST(TestSerializeTSoundSetLevelDirectiveModelWithMultiroom) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetSoundSetLevelDirectiveModelWithMultiroom(),
                                      SOUND_SET_LEVEL_DIRECTIVE_WITH_MULTIROOM);
    }

    Y_UNIT_TEST(TestSerializeTStartMultiroomDirectiveModelWithLocationInfoAndIncludeCurrentDeviceSetTrue) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetStartMultiroomDirectiveModelWithLocationInfoAndIncludeCurrentDevice(),
                                      START_MULTIROOM_DIRECTIVE_WITH_LOCATION_INFO_2);
    }

    Y_UNIT_TEST(TestSerializeTThereminPlayDirectiveModelWithExternalSetModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetThereminPlayDirectiveModelWithExternalSet(),
                                      THEREMIN_PLAY_DIRECTIVE_WITH_EXTERNAL_SET);
    }

    Y_UNIT_TEST(TestSerializeTThereminPlayDirectiveModelWithInternalSetModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetThereminPlayDirectiveModelWithInternalSet(),
                                      THEREMIN_PLAY_DIRECTIVE_WITH_INTERNAL_SET);
    }

    Y_UNIT_TEST(TestSerializeTTypeTextDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetTypeTextDirectiveModel(), TYPE_TEXT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTTypeTextSilentDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetTypeTextSilentDirectiveModel(), TYPE_TEXT_SILENT_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTUpdateDialogInfoDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetUpdateDialogInfoDirectiveModel(), UPDATE_DIALOG_INFO_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTVideoPlayDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetVideoPlayDirectiveModel(), VIDEO_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTYandexNaviDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetYandexNaviDirectiveModel(), YANDEXNAVI_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTCallbackDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetCallbackDirectiveModel(), CALLBACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTGetNextCallbackDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetGetNextCallbackDirectiveModel(), GET_NEXT_CALLBACK_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAddContactBookAsrDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetAddContactBookAsrDirectiveModel(), ADD_CONTACT_BOOK_ASR_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTDeferApplyDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetDeferApplyDirectiveModel(), DEFER_APPLY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAudioPlayDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetAudioPlayDirectiveModel(), AUDIO_PLAY_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTShowButtonsDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetShowButtonsDirectiveModel(), SHOW_BUTTONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTUpdateSpaceActionsDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetUpdateSpaceActionsDirectiveModel(), UPDATE_SPACE_ACTIONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAddConditionalActionsDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetAddConditionalActionsDirectiveModel(), ADD_CONDITIONAL_ACTIONS_DIRECTIVE);
    }

    Y_UNIT_TEST(TestSerializeTAddExternalEntitiesDescriptionDirectiveModel) {
        ASSERT_SERIALIZE_STRUCT_MODEL(GetAddExternalEntitiesDescriptionDirectiveModel(), ADD_EXTERNAL_ENTITIES_DESCRIPTION_DIRECTIVE);
    }
}

} // namespace NAlice::NMegamind
