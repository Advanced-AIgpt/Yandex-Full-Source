#pragma once

#include "meta.h"
#include "speechkit_struct_serializer.h"

#include <alice/megamind/library/models/directives/client_directive_model.h>
#include <alice/megamind/library/models/directives/server_directive_model.h>
#include <alice/megamind/library/models/directives/uniproxy_directive_model.h>
#include <alice/megamind/library/models/interfaces/button_model.h>
#include <alice/megamind/library/models/interfaces/card_model.h>
#include <alice/megamind/library/models/interfaces/directive_model.h>
#include <alice/megamind/library/models/interfaces/model_serializer.h>

#include <alice/megamind/protos/speechkit/directives.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

// Fwd models.
class TProtobufUniproxyDirectiveModel;

class TSpeechKitProtoSerializer final : public virtual IModelSerializer {
public:
    TSpeechKitProtoSerializer() = default;
    explicit TSpeechKitProtoSerializer(TSerializerMeta serializerMeta);

    [[nodiscard]] TSpeechKitResponseProto_TResponse_TButton Serialize(const IButtonModel& model) const;
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TCard Serialize(const ICardModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective Serialize(const IDirectiveModel& model) const;
    [[nodiscard]] TMaybe<NSpeechKit::TProtobufUniproxyDirective> SerializeProtobufUniproxyDirective(const IDirectiveModel& model) const;

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
    void Visit(const TGetNextCallbackDirectiveModel& model) final;
    void Visit(const TDeferApplyDirectiveModel& model) final;
    void Visit(const TUpdateDatasyncDirectiveModel& model) final;
    void Visit(const TAudioPlayDirectiveModel& model) final;
    void Visit(const TAudioPlayDirectiveMetadataModel& model) final;
    void Visit(const TAudioPlayDirectiveStreamModel& model) final;
    void Visit(const TAudioPlayDirectiveCallbacksModel& model) final;
    void Visit(const TAudioPlayDirectiveStreamNormalizationModel& model) final;
    void Visit(const TMusicMetadataModel& model) final;
    void Visit(const TStubInnerGlagolMetadataModel& model) final;
    void Visit(const TGlagolMetadataModel& model) final;
    void Visit(const TUpdateSpaceActionsDirectiveModel& model) final;
    void Visit(const TAddConditionalActionsDirectiveModel& model) final;
    void Visit(const TAddExternalEntitiesDescriptionDirectiveModel& model) final;
    void Visit(const TShowButtonsDirectiveModel& model) final;
    void Visit(const TProtobufUniproxyDirectiveModel& model) final;

private:
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TButton SerializeBaseModel(const IButtonModel& model) const;
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TCard SerializeBaseModel(const ICardModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeBaseModel(const IDirectiveModel& model) const;

    [[nodiscard]] NSpeechKit::TDirective SerializeDirective(const TClientDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeDirective(const TServerDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeDirective(const TUniproxyDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeBaseDirective(const TBaseDirectiveModel& model) const;

    [[nodiscard]] TSpeechKitResponseProto_TResponse_TButton SerializeModel(const TActionButtonModel& model) const;
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TCard SerializeModel(const TDiv2CardModel& model) const;
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TCard SerializeModel(const TDivCardModel& model) const;
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TCard SerializeModel(const TTextCardModel& model) const;
    [[nodiscard]] TSpeechKitResponseProto_TResponse_TCard SerializeModel(const TTextWithButtonCardModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAddContactBookAsrDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAlarmNewDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAlarmSetSoundDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TCloseDialogDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TEndDialogSessionDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TFindContactsDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TMementoChangeUserObjectsDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TMusicPlayDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TOpenDialogDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TOpenSettingsDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TPlayerRewindDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TSetCookiesDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TSetSearchFilterDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TSetTimerDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TThereminPlayDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TTypedSemanticFrameRequestDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TUniversalClientDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TUniversalUniproxyDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TUpdateDialogInfoDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TCallbackDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TGetNextCallbackDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TDeferApplyDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TUpdateDatasyncDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAudioPlayDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAudioPlayDirectiveMetadataModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAudioPlayDirectiveStreamModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAudioPlayDirectiveCallbacksModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAudioPlayDirectiveStreamNormalizationModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TUpdateSpaceActionsDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAddConditionalActionsDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TAddExternalEntitiesDescriptionDirectiveModel& model) const;
    [[nodiscard]] NSpeechKit::TDirective SerializeModel(const TShowButtonsDirectiveModel& model) const;

private:
    TSerializerMeta SerializerMeta;
    TSpeechKitStructSerializer SpeechKitStructSerializer;

    TSpeechKitResponseProto_TResponse_TButton Button;
    TSpeechKitResponseProto_TResponse_TCard Card;
    NSpeechKit::TDirective Directive;
    TMaybe<NSpeechKit::TProtobufUniproxyDirective> ProtobufUniproxyDirective;
};

} // namespace NAlice::NMegamind
