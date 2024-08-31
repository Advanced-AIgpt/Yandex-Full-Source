#pragma once

#include "meta.h"

#include <alice/megamind/library/models/directives/add_conditional_actions.h>
#include <alice/megamind/library/models/directives/add_external_entities_description.h>
#include <alice/megamind/library/models/directives/alarm_set_sound_directive_model.h>
#include <alice/megamind/library/models/directives/client_directive_model.h>
#include <alice/megamind/library/models/directives/get_next_callback_directive_model.h>
#include <alice/megamind/library/models/directives/server_directive_model.h>
#include <alice/megamind/library/models/directives/theremin_play_directive_model.h>
#include <alice/megamind/library/models/directives/uniproxy_directive_model.h>
#include <alice/megamind/library/models/directives/update_dialog_info_directive_model.h>
#include <alice/megamind/library/models/directives/update_space_actions.h>
#include <alice/megamind/library/models/interfaces/button_model.h>
#include <alice/megamind/library/models/interfaces/card_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>
#include <alice/megamind/library/models/interfaces/model_serializer.h>

#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/proto/protobuf.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NMegamind {

class TSpeechKitStructSerializer final : public virtual IModelSerializer {
public:
    TSpeechKitStructSerializer() = default;
    explicit TSpeechKitStructSerializer(TSerializerMeta serializerMeta);

    [[nodiscard]] google::protobuf::Struct Serialize(const IModel& model) const;
    [[nodiscard]] google::protobuf::Struct Serialize(const TAlarmSetSoundDirectiveModel::TSettings& model) const;
    [[nodiscard]] google::protobuf::Struct
    Serialize(const TUpdateSpaceActionsDirectiveModel::TActionSpaces& model) const;
    [[nodiscard]] google::protobuf::Struct
    Serialize(const THashMap<TString, TConditionalAction>& model) const;
    [[nodiscard]] google::protobuf::Struct
    Serialize(const TVector<NData::TExternalEntityDescription>& model) const;

    void Visit(const TActionButtonModel& model) final;
    void Visit(const TDiv2CardModel& model) final;
    void Visit(const TDivCardModel& model) final;
    void Visit(const TTextCardModel& model) final;
    void Visit(const TTextWithButtonCardModel& model) final;
    void Visit(const TAddContactBookAsrDirectiveModel& model) final;
    void Visit(const TAlarmNewDirectiveModel& model) final;
    void Visit(const TAlarmSetSoundDirectiveModel& model) final;
    void Visit(const TCloseDialogDirectiveModel& model) final;
    void Visit(const TEndDialogSessionDirectiveModel& model) final;
    void Visit(const TFindContactsDirectiveModel& model) final;
    void Visit(const TMementoChangeUserObjectsDirectiveModel& model) final;
    void Visit(const TMusicPlayDirectiveModel& model) final;
    void Visit(const TOpenDialogDirectiveModel& model) final;
    void Visit(const TOpenSettingsDirectiveModel& model) final;
    void Visit(const TPlayerRewindDirectiveModel& model) final;
    void Visit(const TSetCookiesDirectiveModel& model) final;
    void Visit(const TSetSearchFilterDirectiveModel& model) final;
    void Visit(const TSetTimerDirectiveModel& model) final;
    void Visit(const TThereminPlayDirectiveModel& model) final;
    void Visit(const TThereminPlayDirectiveExternalSetModel& model) final;
    void Visit(const TThereminPlayDirectiveInternalSetModel& model) final;
    void Visit(const TTypedSemanticFrameRequestDirectiveModel& model) final;
    void Visit(const TUniversalClientDirectiveModel& mode) final;
    void Visit(const TUniversalUniproxyDirectiveModel& mode) final;
    void Visit(const TUpdateDialogInfoDirectiveModel& model) final;
    void Visit(const TUpdateDialogInfoDirectiveMenuItemModel& model) final;
    void Visit(const TUpdateDialogInfoDirectiveStyleModel& model) final;
    void Visit(const TCallbackDirectiveModel& model) final;
    void Visit(const TDeferApplyDirectiveModel& model) final;
    void Visit(const TUpdateDatasyncDirectiveModel& model) final;
    void Visit(const TAudioPlayDirectiveModel& model) final;
    void Visit(const TAudioPlayDirectiveMetadataModel& model) final;
    void Visit(const TAudioPlayDirectiveStreamModel& model) final;
    void Visit(const TAudioPlayDirectiveCallbacksModel& model) final;
    void Visit(const TAudioPlayDirectiveStreamNormalizationModel& model) final;
    void Visit(const TGetNextCallbackDirectiveModel& model) final;
    void Visit(const TStubInnerGlagolMetadataModel& model) final;
    void Visit(const TMusicMetadataModel& model) final;
    void Visit(const TGlagolMetadataModel& model) final;
    void Visit(const TUpdateSpaceActionsDirectiveModel& model) final;
    void Visit(const TShowButtonsDirectiveModel& model) final;
    void Visit(const TAddConditionalActionsDirectiveModel& model) final;
    void Visit(const TAddExternalEntitiesDescriptionDirectiveModel& model) final;
    void Visit(const TProtobufUniproxyDirectiveModel& model) final;

private:
    [[nodiscard]] TProtoStructBuilder SerializeBaseModel(const IButtonModel& model) const;
    [[nodiscard]] TProtoStructBuilder SerializeBaseModel(const ICardModel& model) const;
    [[nodiscard]] TProtoStructBuilder SerializeBaseModel(const IDirectiveModel& model) const;

    [[nodiscard]] TProtoStructBuilder SerializeBaseDirective(const TClientDirectiveModel& model) const;
    [[nodiscard]] TProtoStructBuilder SerializeBaseDirective(const TServerDirectiveModel& model) const;
    [[nodiscard]] TProtoStructBuilder SerializeBaseDirective(const TUniproxyDirectiveModel& model) const;

    [[nodiscard]] google::protobuf::Struct SerializeModel(const TActionButtonModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TDiv2CardModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TDivCardModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TTextCardModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TTextWithButtonCardModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAddContactBookAsrDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAlarmNewDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAlarmSetSoundDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TCloseDialogDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TEndDialogSessionDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TFindContactsDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TMementoChangeUserObjectsDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TMusicPlayDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TOpenDialogDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TOpenSettingsDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TPlayerRewindDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TSetCookiesDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TSetSearchFilterDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TSetTimerDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TThereminPlayDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TThereminPlayDirectiveExternalSetModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TThereminPlayDirectiveInternalSetModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TTypedSemanticFrameRequestDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUniversalClientDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUniversalUniproxyDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUpdateDialogInfoDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUpdateDialogInfoDirectiveStyleModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUpdateDialogInfoDirectiveMenuItemModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TCallbackDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TDeferApplyDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUpdateDatasyncDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAudioPlayDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAudioPlayDirectiveMetadataModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAudioPlayDirectiveStreamModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAudioPlayDirectiveCallbacksModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAudioPlayDirectiveStreamNormalizationModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TGetNextCallbackDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TStubInnerGlagolMetadataModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TMusicMetadataModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TGlagolMetadataModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TUpdateSpaceActionsDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAddConditionalActionsDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TAddExternalEntitiesDescriptionDirectiveModel& model) const;
    [[nodiscard]] google::protobuf::Struct SerializeModel(const TShowButtonsDirectiveModel& model) const;

private:
    TSerializerMeta SerializerMeta;

    google::protobuf::Struct Struct;
};

} // namespace NAlice::NMegamind
