#include "server_action_event.h"

#include <alice/megamind/library/common/defs.h>
#include <alice/megamind/library/serializers/scenario_proto_serializer.h>
#include <alice/megamind/library/util/request.h>

namespace NAlice {

namespace {

const THashMap<TStringBuf, ECallbackType> CALLBACK_TYPE_MAPPING = {
    {TStringBuf("on_suggest"), ECallbackType::OnSuggest},
    {TStringBuf("on_card_action"), ECallbackType::OnCardAction},
    {TStringBuf("on_external_button"), ECallbackType::OnSuggest},
    {NMegamind::SEMANTIC_FRAME_REQUEST_NAME, ECallbackType::SemanticFrame},
    {MM_STACK_ENGINE_GET_NEXT_CALLBACK_NAME, ECallbackType::GetNext},
};

/**
 * SpeechKit creates own 'new_dialog_session' directive on tab opening.
 * Thus 'new_dialog_session' doesn't contain Megamind's fields (i.e. scenario name) in payload.
 * That's why we have to manually process this directive.
 */
NMegamind::TCallbackDirectiveModel ExtractCallbackDirective(const TEvent& event) {
    const auto& name = event.GetName();
    auto payload = event.GetPayload();
    if (name == TStringBuf("new_dialog_session")) {
        auto& fields = *payload.mutable_fields();
        if (fields.count("dialog_id")) {
            fields["dialog_id"].set_string_value(
                NMegamind::SplitDialogId(fields["dialog_id"].string_value()).ScenarioDialogId);
        }
    }
    return {name, event.GetIgnoreAnswer(), payload, /* isLedSilent= */ true};
}

} // namespace

void TServerActionEvent::FillScenarioInput(const TMaybe<TString>& /* normalizedUtterance */,
                                           NScenarios::TInput* input) const {
    *input->MutableCallback() =
        NMegamind::TScenarioProtoSerializer::SerializeDirective(ExtractCallbackDirective(SpeechKitEvent()));
}

ECallbackType TServerActionEvent::GetCallbackType() const {
    if (const auto* callbackType = CALLBACK_TYPE_MAPPING.FindPtr(GetName())) {
        return *callbackType;
    }
    return ECallbackType::None;
}

const TString& TServerActionEvent::GetName() const {
    return SpeechKitEvent().GetName();
}

const google::protobuf::Struct& TServerActionEvent::GetPayload() const {
    return SpeechKitEvent().GetPayload();
}

} // namespace NAlice
