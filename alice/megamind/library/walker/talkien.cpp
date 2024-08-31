#include "talkien.h"

#include <alice/megamind/library/begemot/begemot.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/response/builder.h>
#include <alice/megamind/library/serializers/scenario_proto_deserializer.h>
#include <alice/megamind/library/serializers/speechkit_struct_serializer.h>
#include <alice/megamind/library/util/slot.h>
#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/megamind/protos/speechkit/response.pb.h>

#include <alice/library/experiments/utils.h>
#include <alice/library/json/json.h>
#include <alice/library/metrics/names.h>

#include <util/generic/mapfindptr.h>

namespace NAlice {

namespace {

using NScenarios::TDirective;
using NScenarios::TCallbackDirective;
using NScenarios::TFrameAction;
using NMegamind::TRecognizedActionInfo;

TVector<NSpeechKit::TDirective> SerializeDirectives(
    const ::google::protobuf::RepeatedPtrField<TDirective>& directives,
    const TSpeechKitRequest& request,
    const TMaybe<TIoTUserInfo>& iotUserInfo,
    const ISession& session,
    TRTLogger& logger
) {
    TVector<NSpeechKit::TDirective> serializedDirectives;
    const NMegamind::TSerializerMeta meta(session.GetPreviousScenarioName(), request->GetHeader().GetRequestId(),
                                          request.ClientInfo(),
                                          iotUserInfo,
                                          request->GetRequest().GetSmartHomeInfo());
    const NMegamind::TScenarioProtoDeserializer deserializer(meta, logger);
    const NMegamind::TSpeechKitProtoSerializer serializer(meta);
    for (const auto& directive : directives) {
        if (const auto directiveModel = deserializer.Deserialize(directive); directiveModel) {
            serializedDirectives.push_back(serializer.Serialize(*directiveModel));
        } else {
            LOG_WARN(logger) << "Unable to deserialize provided action directive: \n"
                             << directive.Utf8DebugString() << '\n';
        }
    }
    return serializedDirectives;
}

TVector<NSpeechKit::TDirective> SerializeDirectives(const ::google::protobuf::RepeatedPtrField<TUntypedDirective>& directives) {
    TVector<NSpeechKit::TDirective> serializedDirectives;
    for (const auto& directive : directives) {
        serializedDirectives.push_back(JsonToProto<NSpeechKit::TDirective>(JsonFromProto(directive)));
    }
    return serializedDirectives;
}

void AddFrameToScenario(
    const TString& frameName,
    const TString& scenarioName,
    IScenarioRegistry::TFramesToScenarios& framesToScenarios
) {
    auto& scenarios = framesToScenarios[frameName];
    if (!IsIn(scenarios, scenarioName)) {
        scenarios.push_back(scenarioName);
    }
}

TVector<TTypedSemanticFrame> GetTypedFramesFromRequest(const TVector<TSemanticFrame>& requestFrames) {
    TVector<TTypedSemanticFrame> typedFrames;
    for (const auto& frame : requestFrames) {
        if (frame.HasTypedSemanticFrame()) {
            typedFrames.push_back(frame.GetTypedSemanticFrame());
        } else {
            if (auto typedFrame = TryMakeTypedSemanticFrameFromSemanticFrame(frame);
                typedFrame.Defined())
            {
                typedFrames.push_back(*typedFrame);
            }
        }
    }
    return typedFrames;
}

TMaybe<TSemanticFrameRequestData>
TryGetEffect(const TVector<TTypedSemanticFrame>& requestFrames,
             const google::protobuf::Map<TString, TDeviceState::TActiveActions::TScreenConditionalActions>& screenConditionalActions) {
    for (const auto& [_, conditionalActions] : screenConditionalActions) {
        for (const auto& action : conditionalActions.GetConditionalActions()) {
            const auto& conditionalFrame = action.GetConditionalSemanticFrame();
            const auto& effect = action.GetEffectFrameRequestData();
            if (ProcessConditionalAction(conditionalFrame, requestFrames)) {
                return effect;
            }
        }
    }
    return Nothing();
}

} // namespace

TDirectiveListResponse::TDirectiveListResponse(
    const TVector<NSpeechKit::TDirective>& directives,
    const TSpeechKitRequest& request,
    const NMegamind::IGuidGenerator& guidGenerator
) {
    const auto& requestHeader = request->GetHeader();
    auto& responseHeader = *Response.MutableHeader();
    responseHeader.SetRequestId(requestHeader.GetRequestId());
    responseHeader.SetResponseId(guidGenerator.GenerateGuid());
    responseHeader.SetDialogId(requestHeader.GetDialogId());
    if (requestHeader.HasSequenceNumber()) {
        responseHeader.SetSequenceNumber(requestHeader.GetSequenceNumber());
    }
    (*Response.MutableSessions())[requestHeader.GetDialogId()] = request->GetSession();

    auto& card = *Response.MutableResponse()->AddCards();
    card.SetType("simple_text");

    Response.MutableVoiceResponse()->SetShouldListen(false);
    for (const auto& directive : directives) {
        Response.MutableResponse()->AddDirectives()->CopyFrom(directive);
    }
}

TDirectiveListResponse::TDirectiveListResponse(
    const ::google::protobuf::RepeatedPtrField<TUntypedDirective>& directives,
    const TSpeechKitRequest& request,
    const NMegamind::IGuidGenerator& guidGenerator
)
    : TDirectiveListResponse(SerializeDirectives(directives), request, guidGenerator)
{
}

TDirectiveListResponse::TDirectiveListResponse(
    const ::google::protobuf::RepeatedPtrField<TDirective>& directives,
    const TSpeechKitRequest& request,
    const TMaybe<TIoTUserInfo>& iotUserInfo,
    const ISession& session,
    TRTLogger& logger,
    const NMegamind::IGuidGenerator& guidGenerator
)
    : TDirectiveListResponse(SerializeDirectives(directives, request, iotUserInfo, session, logger),
                             request, guidGenerator)
{
}

TMaybe<TActionResponse> TryGetScenarioActionResponse(const IContext& ctx,
                                                     const TMaybe<TIoTUserInfo>& iotUserInfo,
                                                     NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                     const NMegamind::IGuidGenerator& guidGenerator) {
    const auto* session = ctx.Session();
    if (!session) {
        return Nothing();
    }

    const auto recognizedAction = ctx.Responses().WizardResponse().TryGetRecognizedAction();
    if (!recognizedAction.Defined()) {
        return Nothing();
    }

    for (const auto& [actionId, action] : session->GetActions()) {
        const auto& nluHint = action.GetNluHint();
        if (nluHint.GetFrameName() != recognizedAction->GetName()) {
            continue;
        }

        const TString& prevReqId = ctx.SpeechKitRequest().Proto().GetHeader().GetPrevReqId();
        analyticsInfoBuilder.SetRecognizedScenarioAction(prevReqId, actionId,
                                                         ctx.Session()->GetPreviousProductScenarioName());

        if (action.GetEffectCase() == TFrameAction::EffectCase::kDirectives) {
            return TDirectiveListResponse{action.GetDirectives().GetList(), ctx.SpeechKitRequest(),
                                          iotUserInfo, *session,
                                          ctx.Logger(), guidGenerator};
        } else if (action.GetEffectCase() == TFrameAction::EffectCase::kCallback) {
            return action.GetCallback();
        } else if (action.GetEffectCase() == TFrameAction::EffectCase::kParsedUtterance) {
            return action.GetParsedUtterance();
        } else if (action.GetEffectCase() == TFrameAction::EffectCase::kFrame) {
            return action.GetFrame();
        }

        return *recognizedAction;
    }

    return Nothing();
}

// TODO(the0): Refactor to be consistent with scenario actions.
TMaybe<TActionResponse> TryGetDeviceActionResponse(const IContext& ctx,
                                                   NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                   const NMegamind::IGuidGenerator& guidGenerator) {
    const auto& wizardResponse = ctx.Responses().WizardResponse();
    const auto actionFrame = wizardResponse.TryGetRecognizedAction();
    if (!actionFrame.Defined()) {
        return Nothing();
    }

    const TString actionId(actionFrame->GetName());
    const auto& deviceActions = ctx.SpeechKitRequest()->GetRequest().GetDeviceState().GetActions();
    if (const auto* action = MapFindPtr(deviceActions, actionId); action) {
        analyticsInfoBuilder.SetRecognizedDeviceAction(actionId);
        return TDirectiveListResponse{action->GetDirectives(), ctx.SpeechKitRequest(), guidGenerator};
    }

    return Nothing();
}

TMaybe<TActionResponse> TryGetActiveActionsResponse(const IContext& ctx,
                                                    NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder) {
    const auto& wizardResponse = ctx.Responses().WizardResponse();
    TVector<TSemanticFrameRequestData> actions;
    const auto& semanticFrames = ctx.SpeechKitRequest().DeviceState().GetActiveActions().GetSemanticFrames();
    for (const auto& frame : wizardResponse.GetRequestFrames()) {
        if (const auto it = semanticFrames.find(frame.GetName()); it != semanticFrames.end()) {
            actions.push_back(it->second);
            analyticsInfoBuilder.AddRecognizedSpaceAction(frame.GetName(), it->second.GetAnalytics());
        }
    }
    if (!actions.empty()) {
        return actions;
    }
    return Nothing();
}

TMaybe<TActionResponse> TryGetActionResponse(const IContext& ctx,
                                             const TMaybe<TIoTUserInfo>& iotUserInfo,
                                             NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                             const NMegamind::IGuidGenerator& guidGenerator) {
    if (const auto actionResponse = TryGetActiveActionsResponse(ctx, analyticsInfoBuilder)) {
        return *actionResponse;
    }
    if (const auto actionResponse = TryGetScenarioActionResponse(ctx, iotUserInfo, analyticsInfoBuilder, guidGenerator)) {
        return *actionResponse;
    }
    if (const auto actionResponse = TryGetDeviceActionResponse(ctx, analyticsInfoBuilder, guidGenerator)) {
        return *actionResponse;
    }
    return Nothing();
}

bool ProcessConditionalAction(const TTypedSemanticFrame& activationFrame,
                              const TVector<TTypedSemanticFrame>& requestFrames) {
    google::protobuf::util::MessageDifferencer messageDifferencer;
    const auto typeCase = activationFrame.GetTypeCase();
    if (typeCase == TTypedSemanticFrame::TYPE_NOT_SET) {
        return false;
    }
    const auto& typedActivationFrame = activationFrame.GetReflection()->GetMessage(
        activationFrame,
        activationFrame.GetDescriptor()->FindFieldByNumber(static_cast<int>(typeCase)));

    const auto* descr = typedActivationFrame.GetDescriptor();
    const auto* reflection = typedActivationFrame.GetReflection();
    for (int slotIndex = 0; slotIndex < descr->field_count(); ++slotIndex) {
        const auto* slot = descr->field(slotIndex);
        if (!reflection->HasField(typedActivationFrame, slot)) {
            messageDifferencer.IgnoreField(slot);
        }
    }

    return AnyOf(requestFrames, [&messageDifferencer, &activationFrame] (const TTypedSemanticFrame& frame) {
        //TODO(nkodosov) fill analytics_info
        return messageDifferencer.Compare(activationFrame, frame);
    });
}

TMaybe<TSemanticFrameRequestData>
TryGetConditionalEffect(const IContext& ctx,
                        NMegamind::TMegamindAnalyticsInfoBuilder& /*analyticsInfoBuilder*/) {
    auto typedFrames = GetTypedFramesFromRequest(ctx.Responses().WizardResponse().GetRequestFrames());
    const auto& skr = ctx.SpeechKitRequest();
    if (const auto effect = TryGetEffect(typedFrames, skr.DeviceState().GetActiveActions().GetScreenConditionalActions())) {
        const auto& analytics = effect->GetAnalytics();
        ctx.Sensors().IncRate(NSignal::LabelsForActivatedConditionalAction(analytics.GetProductScenario(), analytics.GetPurpose()));
        return effect;
    }
    for (const auto& environmentDeviceInfo : skr->GetRequest().GetEnvironmentState().GetDevices()) {
        if (environmentDeviceInfo.HasSpeakerDeviceState()) {
            if (const auto effect = TryGetEffect(typedFrames, environmentDeviceInfo.GetSpeakerDeviceState()
                                                                                   .GetActiveActions()
                                                                                   .GetScreenConditionalActions()))
            {
                const auto& analytics = effect->GetAnalytics();
                ctx.Sensors().IncRate(NSignal::LabelsForActivatedConditionalAction(analytics.GetProductScenario(), analytics.GetPurpose()));
                return effect;
            }
        }
    }
    return Nothing();
}

IScenarioRegistry::TFramesToScenarios AddDynamicAcceptedFrames(
    IScenarioRegistry::TFramesToScenarios framesToScenarios,
    const ISession* session
) {
    if (!session || session->GetPreviousScenarioName().empty()) {
        return framesToScenarios;
    }
    const auto& scenarioName = session->GetPreviousScenarioName();
    for (const auto& [_, action] : session->GetActions()) {
        // NOTE(the0): Here we assume an action with no effect to be a valid action. It serves as a hint for begemot and turns
        // into a frame to be passed together with other recognized frames as input to scenarios.
        // THIS BEHAVIOUR IS EXPERIMENTAL, NOT GUARANTIEED TO BE SUPPORTED AND SHOULD NOT BE CONSIDERED AS A PART OF PUBLIC API.
        AddFrameToScenario(action.GetNluHint().GetFrameName(), scenarioName, framesToScenarios);
    }
    if (const auto responseFrame = session->GetResponseFrame();
        responseFrame.Defined() && GetRequestedSlot(*responseFrame).Defined())
    {
        AddFrameToScenario(responseFrame->GetName(), scenarioName, framesToScenarios);
    }
    return framesToScenarios;
}

} // namespace NAlice
