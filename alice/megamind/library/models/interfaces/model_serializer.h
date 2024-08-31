#pragma once

namespace NAlice::NMegamind {

// ButtonModels
class TActionButtonModel;

// CardModels
class TDiv2CardModel;
class TDivCardModel;
class TTextCardModel;
class TTextWithButtonCardModel;

// DirectiveModels
class TAddConditionalActionsDirectiveModel;
class TAddContactBookAsrDirectiveModel;
class TAddExternalEntitiesDescriptionDirectiveModel;
class TAlarmNewDirectiveModel;
class TAlarmSetSoundDirectiveModel;
class TAudioPlayDirectiveCallbacksModel;
class TAudioPlayDirectiveMetadataModel;
class TAudioPlayDirectiveModel;
class TAudioPlayDirectiveStreamModel;
class TAudioPlayDirectiveStreamNormalizationModel;
class TCallbackDirectiveModel;
class TCloseDialogDirectiveModel;
class TDeferApplyDirectiveModel;
class TEndDialogSessionDirectiveModel;
class TFindContactsDirectiveModel;
class TGetNextCallbackDirectiveModel;
class TGlagolMetadataModel;
class TMementoChangeUserObjectsDirectiveModel;
class TMusicMetadataModel;
class TMusicPlayDirectiveModel;
class TOpenDialogDirectiveModel;
class TOpenSettingsDirectiveModel;
class TPlayerRewindDirectiveModel;
class TProtobufUniproxyDirectiveModel;
class TSetCookiesDirectiveModel;
class TSetSearchFilterDirectiveModel;
class TSetTimerDirectiveModel;
class TShowButtonsDirectiveModel;
class TStubInnerGlagolMetadataModel;
class TThereminPlayDirectiveExternalSetModel;
class TThereminPlayDirectiveInternalSetModel;
class TThereminPlayDirectiveModel;
class TTypedSemanticFrameRequestDirectiveModel;
class TUniversalClientDirectiveModel;
class TUniversalUniproxyDirectiveModel;
class TUpdateDatasyncDirectiveModel;
class TUpdateDialogInfoDirectiveMenuItemModel;
class TUpdateDialogInfoDirectiveModel;
class TUpdateDialogInfoDirectiveStyleModel;
class TUpdateSpaceActionsDirectiveModel;

class IModelSerializer {
public:
    virtual ~IModelSerializer() = default;

    // ButtonModels
    virtual void Visit(const TActionButtonModel& model) = 0;

    // CardModels
    virtual void Visit(const TDiv2CardModel& model) = 0;
    virtual void Visit(const TDivCardModel& model) = 0;
    virtual void Visit(const TTextCardModel& model) = 0;
    virtual void Visit(const TTextWithButtonCardModel& model) = 0;

    // ClientDirectiveModels
    virtual void Visit(const TAddConditionalActionsDirectiveModel& model) = 0;
    virtual void Visit(const TAddContactBookAsrDirectiveModel& model) = 0;
    virtual void Visit(const TAddExternalEntitiesDescriptionDirectiveModel& model) = 0;
    virtual void Visit(const TAlarmNewDirectiveModel& model) = 0;
    virtual void Visit(const TAlarmSetSoundDirectiveModel& model) = 0;
    virtual void Visit(const TAudioPlayDirectiveCallbacksModel& model) = 0;
    virtual void Visit(const TAudioPlayDirectiveMetadataModel& model) = 0;
    virtual void Visit(const TAudioPlayDirectiveModel& model) = 0;
    virtual void Visit(const TAudioPlayDirectiveStreamModel& model) = 0;
    virtual void Visit(const TAudioPlayDirectiveStreamNormalizationModel& model) = 0;
    virtual void Visit(const TCloseDialogDirectiveModel& model) = 0;
    virtual void Visit(const TEndDialogSessionDirectiveModel& model) = 0;
    virtual void Visit(const TFindContactsDirectiveModel& model) = 0;
    virtual void Visit(const TGlagolMetadataModel& model) = 0;
    virtual void Visit(const TMusicMetadataModel& model) = 0;
    virtual void Visit(const TMusicPlayDirectiveModel& model) = 0;
    virtual void Visit(const TOpenDialogDirectiveModel& model) = 0;
    virtual void Visit(const TOpenSettingsDirectiveModel& model) = 0;
    virtual void Visit(const TPlayerRewindDirectiveModel& model) = 0;
    virtual void Visit(const TSetCookiesDirectiveModel& model) = 0;
    virtual void Visit(const TSetSearchFilterDirectiveModel& model) = 0;
    virtual void Visit(const TSetTimerDirectiveModel& model) = 0;
    virtual void Visit(const TStubInnerGlagolMetadataModel& model) = 0;
    virtual void Visit(const TThereminPlayDirectiveModel& model) = 0;
    virtual void Visit(const TThereminPlayDirectiveExternalSetModel& model) = 0;
    virtual void Visit(const TThereminPlayDirectiveInternalSetModel& model) = 0;
    virtual void Visit(const TUniversalClientDirectiveModel& model) = 0;
    virtual void Visit(const TUniversalUniproxyDirectiveModel& model) = 0;
    virtual void Visit(const TUpdateDialogInfoDirectiveModel& model) = 0;
    virtual void Visit(const TUpdateDialogInfoDirectiveMenuItemModel& model) = 0;
    virtual void Visit(const TUpdateDialogInfoDirectiveStyleModel& model) = 0;
    virtual void Visit(const TUpdateSpaceActionsDirectiveModel& model) = 0;
    virtual void Visit(const TShowButtonsDirectiveModel& model) = 0;

    // ServerDirectiveModels
    virtual void Visit(const TCallbackDirectiveModel& model) = 0;
    virtual void Visit(const TGetNextCallbackDirectiveModel& model) = 0;
    virtual void Visit(const TTypedSemanticFrameRequestDirectiveModel& model) = 0;

    // UniproxyDirectiveModels
    virtual void Visit(const TDeferApplyDirectiveModel& model) = 0;
    virtual void Visit(const TMementoChangeUserObjectsDirectiveModel& model) = 0;
    virtual void Visit(const TUpdateDatasyncDirectiveModel& model) = 0;

    // ProtobufUniproxyDirective
    virtual void Visit(const TProtobufUniproxyDirectiveModel& model) = 0;
};

} // namespace NAlice::NMegamind
