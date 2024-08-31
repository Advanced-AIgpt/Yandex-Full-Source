#pragma once

#include "meta.h"
#include "speechkit_struct_serializer.h"

#include <alice/megamind/library/models/buttons/action_button_model.h>
#include <alice/megamind/library/models/cards/text_card_model.h>
#include <alice/megamind/library/models/cards/text_with_button_card_model.h>
#include <alice/megamind/library/models/directives/alarm_new_directive_model.h>
#include <alice/megamind/library/models/directives/alarm_set_sound_directive_model.h>
#include <alice/megamind/library/models/directives/audio_play_directive_model.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/models/directives/close_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/end_dialog_session_directive_model.h>
#include <alice/megamind/library/models/directives/find_contacts_directive_model.h>
#include <alice/megamind/library/models/directives/music_play_directive_model.h>
#include <alice/megamind/library/models/directives/open_dialog_directive_model.h>
#include <alice/megamind/library/models/directives/open_settings_directive_model.h>
#include <alice/megamind/library/models/directives/player_rewind_directive_model.h>
#include <alice/megamind/library/models/directives/set_cookies_directive_model.h>
#include <alice/megamind/library/models/directives/set_search_filter_directive_model.h>
#include <alice/megamind/library/models/directives/set_timer_directive_model.h>
#include <alice/megamind/library/models/directives/show_buttons_directive_model.h>
#include <alice/megamind/library/models/directives/theremin_play_directive_model.h>
#include <alice/megamind/library/models/directives/universal_client_directive_model.h>
#include <alice/megamind/library/models/directives/universal_uniproxy_directive_model.h>
#include <alice/megamind/library/models/directives/update_dialog_info_directive_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>

#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/layout.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/logger/logger.h>
#include <alice/protos/extensions/extensions.pb.h>

#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NAlice::NMegamind {

// Fwd models.
class TProtobufUniproxyDirectiveModel;

class TScenarioProtoDeserializer {
public:
    TScenarioProtoDeserializer() = default;
    explicit TScenarioProtoDeserializer(TSerializerMeta serializerMeta,
                                        TRTLogger& logger,
                                        THashMap<TString, NScenarios::TFrameAction>&& actions = {},
                                        THashMap<TString, NScenarios::TActionSpace>&& actionSpaces = {},
                                        TMaybe<TString>&& fillCloudUiDirectiveText = Nothing());

    TIntrusivePtr<ICardModel> Deserialize(const NScenarios::TLayout_TCard& card) const;
    TIntrusivePtr<IDirectiveModel> Deserialize(const NScenarios::TDirective& directive) const;
    TIntrusivePtr<IDirectiveModel> Deserialize(const NScenarios::TServerDirective& directive) const;
    TIntrusivePtr<IButtonModel> Deserialize(const NScenarios::TLayout_TButton& button,
                                            const bool fromSuggest = false) const;
    TIntrusivePtr<IButtonModel> Deserialize(const NScenarios::TLayout_TSuggest& suggest) const;
    TIntrusivePtr<IButtonModel> Deserialize(const NScenarios::TButton& button) const;

    TIntrusivePtr<TTextWithButtonCardModel> Deserialize(const NScenarios::TLayout_TTextWithButtons& card) const;
    TIntrusivePtr<TAlarmNewDirectiveModel> Deserialize(const NScenarios::TAlarmNewDirective& directive) const;
    TIntrusivePtr<TAlarmSetSoundDirectiveModel>
    Deserialize(const NScenarios::TAlarmSetSoundDirective& directive) const;
    TIntrusivePtr<TCallbackDirectiveModel> Deserialize(const NScenarios::TCallbackDirective& directive) const;
    TIntrusivePtr<TCloseDialogDirectiveModel> Deserialize(const NScenarios::TCloseDialogDirective& directive) const;
    TIntrusivePtr<TFindContactsDirectiveModel> Deserialize(const NScenarios::TFindContactsDirective& directive) const;
    TIntrusivePtr<TEndDialogSessionDirectiveModel>
    Deserialize(const NScenarios::TEndDialogSessionDirective& directive) const;
    TIntrusivePtr<TMusicPlayDirectiveModel> Deserialize(const NScenarios::TMusicPlayDirective& directive) const;
    TIntrusivePtr<TOpenDialogDirectiveModel> Deserialize(const NScenarios::TOpenDialogDirective& directive) const;
    TIntrusivePtr<TOpenSettingsDirectiveModel> Deserialize(const NScenarios::TOpenSettingsDirective& directive) const;
    TIntrusivePtr<TPlayerRewindDirectiveModel> Deserialize(const NScenarios::TPlayerRewindDirective& directive) const;
    TIntrusivePtr<TSetCookiesDirectiveModel> Deserialize(const NScenarios::TSetCookiesDirective& directive) const;
    TIntrusivePtr<TSetSearchFilterDirectiveModel>
    Deserialize(const NScenarios::TSetSearchFilterDirective& directive) const;
    TIntrusivePtr<TSetTimerDirectiveModel> Deserialize(const NScenarios::TSetTimerDirective& directive) const;
    TIntrusivePtr<TThereminPlayDirectiveModel> Deserialize(const NScenarios::TThereminPlayDirective& directive) const;
    TIntrusivePtr<TUpdateDialogInfoDirectiveModel>
    Deserialize(const NScenarios::TUpdateDialogInfoDirective& directive) const;
    TIntrusivePtr<TShowButtonsDirectiveModel> Deserialize(const NScenarios::TShowButtonsDirective& directive) const;
    TIntrusivePtr<TUniversalClientDirectiveModel> Deserialize(const NScenarios::TRequestPermissionsDirective& directive) const;
    TIntrusivePtr<TUniversalClientDirectiveModel>
    Deserialize(const NScenarios::TMultiroomSemanticFrameDirective& directive) const;
    TIntrusivePtr<TUniversalClientDirectiveModel> Deserialize(const NScenarios::TStartMultiroomDirective& directive) const;

    // Reminders related stuff.
    TIntrusivePtr<TUniversalClientDirectiveModel> Deserialize(const NScenarios::TRemindersSetDirective& directive) const;
    TIntrusivePtr<TUniversalClientDirectiveModel> Deserialize(const NScenarios::TRemindersCancelDirective& directive) const;

    TIntrusivePtr<TTypedSemanticFrameRequestDirectiveModel> Deserialize(const TSemanticFrameRequestData& typedFrame) const;

    TActionButtonModel::TTheme Deserialize(const NScenarios::TLayout_TButton_TTheme& theme) const;
    TActionButtonModel::TTheme Deserialize(const NScenarios::TLayout_TSuggest_TActionButton_TTheme& theme) const;
    TActionButtonModel::TTheme Deserialize(const NScenarios::TButton_TTheme& theme) const;
    TUpdateDialogInfoDirectiveMenuItemModel
    Deserialize(const NScenarios::TUpdateDialogInfoDirective_TMenuItem& menuItem) const;
    TUpdateDialogInfoDirectiveStyleModel Deserialize(const NScenarios::TUpdateDialogInfoDirective_TStyle& style) const;
    TIntrusivePtr<TAudioPlayDirectiveModel> Deserialize(const NScenarios::TAudioPlayDirective& directive) const;

    // Server Directives
    TIntrusivePtr<TUniproxyDirectiveModel> Deserialize(const NScenarios::TUpdateDatasyncDirective& directive) const;

    // Matrix scheduler related stuff.
    TIntrusivePtr<TProtobufUniproxyDirectiveModel>
    Deserialize(const NScenarios::TCancelScheduledActionDirective& directive) const;

    TIntrusivePtr<TProtobufUniproxyDirectiveModel>
    Deserialize(const NScenarios::TEnlistScheduledActionDirective& directive) const;

    void CreateDeepLinks(google::protobuf::Struct& body, const bool addOnSuggestDirective = true) const;

private:
    TIntrusivePtr<TUniproxyDirectiveModel> TryDeserializeUniproxyDirective(const NScenarios::TDirective& directive) const;
    TIntrusivePtr<TBaseDirectiveModel> TryDeserializeBaseDirective(const NScenarios::TDirective& directive) const;
    TIntrusivePtr<TBaseDirectiveModel> TryDeserializeBaseDirectiveImpl(const NScenarios::TDirective& directive) const;
    TIntrusivePtr<ICardModel> DeserializeDivCard(google::protobuf::Struct body) const;
    TIntrusivePtr<ICardModel> DeserializeDiv2Card(google::protobuf::Struct body, bool hideBorders, TString text) const;
    TVector<TIntrusivePtr<IDirectiveModel>> GenerateDirectives(const TString& actionId, const TMaybe<TString>& suggestTitle = Nothing()) const;

    TVector<TIntrusivePtr<IDirectiveModel>> GenerateActionSpaceDirectives(
        const TString& actionId, const TString& onSuggestCaption,
        const google::protobuf::Map<TString, NScenarios::TActionSpace::TAction>& actions) const;

    TIntrusivePtr<TUniversalClientDirectiveModel>
    DeserializeUniversalClientDirective(const google::protobuf::Message& directive) const;
    TIntrusivePtr<TUniversalUniproxyDirectiveModel>
    DeserializeUniversalUniproxyDirective(const TString& name, const NScenarios::TDirective& directive) const;
    TIntrusivePtr<TUniversalUniproxyDirectiveModel>
    DeserializeUniversalUniproxyDirective(const TString& name, const NScenarios::TServerDirective& directive) const;
    TIntrusivePtr<TUniversalUniproxyDirectiveModel>
    DeserializePushUniproxyDirective(const TString& name, const NScenarios::TServerDirective& directive) const;
    TIntrusivePtr<TUniversalUniproxyDirectiveModel> DeserializePushTypedSemanticFrameDirective(
        const NScenarios::TPushTypedSemanticFrameDirective& directive) const;
    TIntrusivePtr<TMementoChangeUserObjectsDirectiveModel> DeserializeMementoChangeUserObjectsDirective(
        const NScenarios::TMementoChangeUserObjectsDirective& directive) const;

    TIntrusivePtr<TUniversalClientDirectiveModel>
    DeserializeNavigateBrowserDirective(const NScenarios::TNavigateBrowserDirective& directive) const;

    TIntrusivePtr<IButtonModel>
    DeserializeSearchButton(const NScenarios::TLayout_TSuggest_TSearchButton& button) const;

    TIntrusivePtr<TUniversalClientDirectiveModel>
    Deserialize(const NScenarios::TMordoviaShowDirective& directive) const;

    TIntrusivePtr<TUniversalClientDirectiveModel>
    Deserialize(const NScenarios::TMordoviaCommandDirective& directive) const;

    TIntrusivePtr<TUniversalClientDirectiveModel>
    Deserialize(NScenarios::TAddCardDirective directive) const;

    TIntrusivePtr<IDirectiveModel>
    DeserializeServerDirective(const NScenarios::TServerDirective& directive) const;

    TMaybe<NSpeechKit::TUniproxyDirectiveMeta>
    TryConstructUniproxyDirectiveMeta(const NScenarios::TServerDirective& directive) const;

private:
    TSerializerMeta SerializerMeta;
    TRTLogger& Logger;
    THashMap<TString, NScenarios::TFrameAction> Actions;
    THashMap<TString, NScenarios::TActionSpace> ActionSpaces;
    TMaybe<TString> FillCloudUiDirectiveText;
    TSpeechKitStructSerializer StructSerializer;
};

} // namespace NAlice::NMegamind
