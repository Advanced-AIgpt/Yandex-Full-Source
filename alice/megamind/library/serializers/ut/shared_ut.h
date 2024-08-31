#pragma once

#include <alice/megamind/library/serializers/scenario_proto_deserializer.h>
#include <alice/megamind/library/serializers/speechkit_proto_serializer.h>
#include <alice/megamind/library/serializers/speechkit_struct_serializer.h>

#include <alice/megamind/library/models/directives/open_settings_directive_model.h>
#include <alice/megamind/library/models/directives/player_rewind_directive_model.h>
#include <alice/megamind/library/models/directives/set_search_filter_directive_model.h>

#include <google/protobuf/struct.pb.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NAlice::NMegamind {

class TFixture : public NUnitTest::TBaseFixture {
public:
    TFixture();

    [[nodiscard]] const TSerializerMeta& GetSerializerMeta() const;

    [[nodiscard]] const TScenarioProtoDeserializer& GetScenarioProtoDeserializer() const;

    [[nodiscard]] const TSpeechKitProtoSerializer& GetSpeechKitProtoModelSerializer() const;
    [[nodiscard]] const TSpeechKitStructSerializer& GetSpeechKitStructModelSerializer() const;

    [[nodiscard]] const TActionButtonModel& GetActionButtonModel() const;
    [[nodiscard]] const TActionButtonModel& GetThemedActionButtonModel() const;
    [[nodiscard]] const TDiv2CardModel& GetDiv2CardModel() const;
    [[nodiscard]] const TDivCardModel& GetDivCardModel() const;
    [[nodiscard]] const TTextCardModel& GetTextCardModel() const;
    [[nodiscard]] const TTextWithButtonCardModel& GetTextWithButtonCardModel() const;
    [[nodiscard]] const TAlarmNewDirectiveModel& GetAlarmNewDirectiveModel() const;
    [[nodiscard]] const TAlarmSetSoundDirectiveModel& GetAlarmSetSoundDirectiveModel() const;
    [[nodiscard]] const TAudioPlayDirectiveModel& GetAudioPlayDirectiveModel() const;
    [[nodiscard]] const TCloseDialogDirectiveModel& GetCloseDialogDirectiveModel() const;
    [[nodiscard]] const TCloseDialogDirectiveModel& GetCloseDialogDirectiveModelWithScreenId() const;
    [[nodiscard]] const TEndDialogSessionDirectiveModel& GetEndDialogSessionDirectiveModel() const;
    [[nodiscard]] const TFindContactsDirectiveModel& GetFindContactsDirectiveModel() const;
    [[nodiscard]] const TMusicPlayDirectiveModel& GetMusicPlayDirectiveModel() const;
    [[nodiscard]] const TMusicPlayDirectiveModel& GetMusicPlayDirectiveModelWithEndpointId() const;
    [[nodiscard]] const TMusicPlayDirectiveModel& GetMusicPlayDirectiveModel2() const;
    [[nodiscard]] const TMusicPlayDirectiveModel& GetMusicPlayDirectiveModelEmptyRoom() const;
    [[nodiscard]] const TMusicPlayDirectiveModel& GetMusicPlayDirectiveModelWithLocationInfo() const;
    [[nodiscard]] const TOpenDialogDirectiveModel& GetOpenDialogDirectiveModel() const;
    [[nodiscard]] const TOpenSettingsDirectiveModel& GetOpenSettingsDirectiveModel(ESettingsTarget target) const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetOpenUriDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetOpenUriDirectiveModelWithScreenId() const;
    [[nodiscard]] const TPlayerRewindDirectiveModel& GetPlayerRewindDirectiveModel(EPlayerRewindType rewindType) const;
    [[nodiscard]] const TSetCookiesDirectiveModel& GetSetCookiesDirectiveModel() const;
    [[nodiscard]] const TSetSearchFilterDirectiveModel&
    GetSetSearchFilterDirectiveModel(ESearchFilterLevel level) const;
    [[nodiscard]] const TSetTimerDirectiveModel& GetSetTimerDirectiveModel(bool setTimestamp) const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetShowGalleryDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetShowPayPushScreenDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetShowSeasonGalleryDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetShowVideoDescriptionDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetCarDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetSoundSetLevelDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetSoundSetLevelInLocationDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetSoundSetLevelDirectiveModelWithMultiroom() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetStartMultiroomDirectiveModelWithLocationInfoAndIncludeCurrentDevice() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetYandexNaviDirectiveModel() const;
    [[nodiscard]] const TThereminPlayDirectiveModel& GetThereminPlayDirectiveModelWithExternalSet() const;
    [[nodiscard]] const TThereminPlayDirectiveModel& GetThereminPlayDirectiveModelWithInternalSet() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetTypeTextDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetTypeTextSilentDirectiveModel() const;
    [[nodiscard]] const TUpdateDialogInfoDirectiveModel& GetUpdateDialogInfoDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetVideoPlayDirectiveModel() const;
    [[nodiscard]] const TCallbackDirectiveModel& GetCallbackDirectiveModel() const;
    [[nodiscard]] const TCallbackDirectiveModel& GetCallbackDirectiveModelWithMultiroom() const;
    [[nodiscard]] const TGetNextCallbackDirectiveModel& GetGetNextCallbackDirectiveModel() const;
    [[nodiscard]] const TAddContactBookAsrDirectiveModel& GetAddContactBookAsrDirectiveModel() const;
    [[nodiscard]] const TDeferApplyDirectiveModel& GetDeferApplyDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetVisualGoBackwardDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetHistoricalGoBackwardDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetNativeGoBackwardDirectiveModel() const;
    [[nodiscard]] const TShowButtonsDirectiveModel& GetShowButtonsDirectiveModel() const;
    [[nodiscard]] const TUniversalClientDirectiveModel& GetSetGlagolMetadataModel() const;
    [[nodiscard]] const TUpdateSpaceActionsDirectiveModel& GetUpdateSpaceActionsDirectiveModel() const;
    [[nodiscard]] const TAddConditionalActionsDirectiveModel& GetAddConditionalActionsDirectiveModel() const;
    [[nodiscard]] const TAddExternalEntitiesDescriptionDirectiveModel& GetAddExternalEntitiesDescriptionDirectiveModel() const;

private:
    TSerializerMeta SerializerMeta;

    TScenarioProtoDeserializer ScenarioProtoDeserializer;

    TSpeechKitProtoSerializer SpeechKitProtoModelSerializer;
    TSpeechKitStructSerializer SpeechKitStructModelSerializer;
};

TStringBuf GetOpenSettingsSkJsonString(ESettingsTarget target);

} // namespace NAlice::NMegamind
