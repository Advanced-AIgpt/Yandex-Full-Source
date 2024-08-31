#include "scenario_wrapper.h"

#include "scenario_api_helper.h"

#include <alice/megamind/library/scenarios/interface/protocol_scenario.h>

#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/models/cards/div_card_model.h>
#include <alice/megamind/library/models/directives/add_conditional_actions.h>
#include <alice/megamind/library/models/directives/add_contact_book_asr_directive_model.h>
#include <alice/megamind/library/models/directives/callback_directive_model.h>
#include <alice/megamind/library/models/directives/memento_change_user_objects_directive_model.h>
#include <alice/megamind/library/models/directives/typed_semantic_frame_request_directive_model.h>
#include <alice/megamind/library/models/directives/update_space_actions.h>
#include <alice/megamind/library/models/interfaces/card_model.h>
#include <alice/megamind/library/request/event/server_action_event.h>
#include <alice/megamind/library/scenarios/protocol/helpers.h>
#include <alice/megamind/library/serializers/meta.h>
#include <alice/megamind/library/serializers/scenario_proto_deserializer.h>

#include <alice/megamind/protos/common/conditional_action.pb.h>
#include <alice/megamind/protos/common/content_properties.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>
#include <alice/megamind/protos/speechkit/request.pb.h>

#include <alice/library/proto/protobuf.h>
#include <alice/library/response/defs.h>
#include <alice/library/version/version.h>

#include <util/generic/hash_set.h>
#include <util/generic/mapfindptr.h>
#include <util/generic/maybe.h>
#include <util/system/yassert.h>

namespace NAlice {

namespace NImpl {

NMegamind::TGetNextCallbackDirectiveModel
CreateGetNextDirectiveFromStackEngine(const NMegamind::IStackEngine& stackEngine,
                                      const NMegamind::TScenarioProtoDeserializer& deserializer) {
    Y_ENSURE(!stackEngine.IsEmpty());
    const auto& stackHead = stackEngine.Peek();
    auto recoveryCallback = stackHead.HasRecoveryAction()
                                ? TMaybe<NMegamind::TCallbackDirectiveModel>{std::move(
                                      *deserializer.Deserialize(stackHead.GetRecoveryAction().GetCallback()))}
                                : Nothing();
    return NMegamind::TGetNextCallbackDirectiveModel{/* ignoreAnswer= */ false,
                                                     /* isLedSilent= */ true, stackEngine.GetSessionId(),
                                                     stackEngine.GetProductScenarioName(),
                                                     std::move(recoveryCallback)};
}

NMegamind::TGetNextCallbackDirectiveModel
CreateGetNextDirectiveFromStackEngine(const NMegamind::IStackEngine& stackEngine,
                                      const NMegamind::TSerializerMeta& serializerMeta, TRTLogger& logger) {
    NMegamind::TScenarioProtoDeserializer deserializer{serializerMeta, logger};
    return CreateGetNextDirectiveFromStackEngine(stackEngine, deserializer);
}

} // namespace NImpl

using TFrameAction = NScenarios::TFrameAction;
using TScenarioFeatures = NMegamind::TScenarioFeatures;

namespace {

void EnrichProtocolInfo(TSessionProto::TProtocolInfo& protocolInfo, const TRequest& request) {
    protocolInfo.SetRequestSourceType(request.GetRequestSource());
    if (const auto& shouldListen = request.GetParameters().GetForcedShouldListen(); shouldListen.Defined()) {
        protocolInfo.MutableForcedShouldListen()->set_value(*shouldListen);
    }
    if (const auto& channel = request.GetParameters().GetChannel(); channel.Defined()) {
        protocolInfo.SetChannel(*channel);
    }
    if (const auto& forcedEmotion = request.GetParameters().GetForcedEmotion(); forcedEmotion.Defined()) {
        protocolInfo.SetForcedEmotion(*forcedEmotion);
    }
}

static const THashSet<NScenarios::TDirective::DirectiveCase> VOICE_RESPONSE_DIRECTIVES{
    NScenarios::TDirective::kUpdateNotificationSubscriptionDirective,
    NScenarios::TDirective::kMarkNotificationAsReadDirective,
    NScenarios::TDirective::kSaveVoiceprintDirective,
    NScenarios::TDirective::kRemoveVoiceprintDirective
};

struct TModeSetter {
    explicit TModeSetter(TMaybe<TScenario::EApplyMode>& mode)
        : Mode(mode)
    {
    }

    TStatus operator()(const TScenario::EApplyMode mode) const {
        Mode = mode;
        return Success();
    }

    TStatus operator()(const TError& error) const {
        return error;
    }

    TMaybe<TScenario::EApplyMode>& Mode;
};

TError ResponseNotSetErrorFor(const TString& scenarioName, const TString& method) {
    return TError{TError::EType::Logic} << method << " method for " << scenarioName << " did not set response";
}

const TString PATH_APPLY{"apply"};
const TString PATH_COMMIT{"commit"};
const TString PATH_CONTINUE{"continue"};
const TString PATH_RUN{"run"};

template<typename TScenarioResponseType>
inline TStringBuilder LogScenarioResponseBase(const TString& scenarioName, const TScenarioResponseType& response) {
    return TStringBuilder{} << "Got response from scenario " << scenarioName << ": " << response;
}

template <typename TScenarioResponseType>
inline TStringBuilder LogScenarioResponse(const TString& scenarioName, const TScenarioResponseType& response,
                                          bool hideSensitiveData = true) {
    if (hideSensitiveData &&
        (response.GetResponseBody().GetLayout().GetContainsSensitiveData() ||
         response.GetResponseBody().GetLayout().GetContentProperties().GetContainsSensitiveDataInResponse()))
    {
        TScenarioResponseType responseToLog(response);
        auto* layout = responseToLog.MutableResponseBody()->MutableLayout();
        layout->Clear();
        layout->MutableContentProperties()->CopyFrom(response.GetResponseBody().GetLayout().GetContentProperties());
        return LogScenarioResponseBase(scenarioName, responseToLog);
    }
    return LogScenarioResponseBase(scenarioName, response);
}

template<>
inline TStringBuilder LogScenarioResponse<NScenarios::TScenarioCommitResponse>(
        const TString& scenarioName,
        const NScenarios::TScenarioCommitResponse& response,
        bool /*hideSensitiveData*/) {
    return LogScenarioResponseBase(scenarioName, response);
}

inline bool IsNewScenarioSession(const ISession* session, const TString& scenarioName) {
    if (!session) {
        return true;
    }
    return session->GetPreviousScenarioName() != scenarioName;
}

inline bool IsSessionReset(const TLightScenarioEnv& env) {
    return env.Ctx.SpeechKitRequest().GetResetSession();
}

void ProcessStackEngine(NMegamind::IStackEngine& stackEngine, const TString& scenarioName,
                        const NScenarios::TStackEngine& scenarioStackEngine,
                        const TString& requestId, const TString& productScenarioName) {
    for (const auto& action : scenarioStackEngine.GetActions()) {
        switch (action.GetActionCase()) {
            case NScenarios::TStackEngineAction::ActionCase::kResetAdd: {
                stackEngine.PopScenario(scenarioName);
                const auto& resetAdd = action.GetResetAdd();
                for (const auto& effect : resetAdd.GetEffects()) {
                    NMegamind::IStackEngine::TItem item{};
                    item.SetScenarioName(scenarioName);
                    item.MutableEffect()->CopyFrom(effect);
                    if (resetAdd.HasRecoveryAction()) {
                        item.MutableRecoveryAction()->CopyFrom(resetAdd.GetRecoveryAction());
                    }
                    stackEngine.Push(std::move(item));
                }
                break;
            }
            case NScenarios::TStackEngineAction::ActionCase::kNewSession:
                stackEngine.StartNewSession(requestId, productScenarioName, scenarioName);
                break;
            case NScenarios::TStackEngineAction::ActionCase::ACTION_NOT_SET:
                break;
        }
    }
}

NMegamind::TUpdateSpaceActionsDirectiveModel
MakeUpdateSpaceActionDirective(const google::protobuf::Map<TString, NScenarios::TActionSpace>& actionSpaces,
                               TRTLogger& logger) {
    NMegamind::TUpdateSpaceActionsDirectiveModel directive{};
    for (const auto& [spaceId, space] : actionSpaces) {
        THashMap<TString, NMegamind::TTypedSemanticFrameRequestDirectiveModel> frameRequests{};
        const auto& actions = space.GetActions();
        for (const auto& hint : space.GetNluHints()) {
            if (hint.GetSemanticFrameName().empty()) {
                LOG_ERR(logger) << "Update space actions: nlu hint has empty semantic frame name";
                continue;
            }
            const auto actionIterator = actions.find(hint.GetActionId());
            if (actionIterator == actions.cend()) {
                LOG_ERR(logger) << "Update space actions: nlu hint referred to unknown action id: " << hint.GetActionId();
                continue;
            }
            const auto& action = actionIterator->second;
            if (!action.HasSemanticFrame()) {
                LOG_ERR(logger) << "Update space actions: nlu hint referred to action without semantic frame";
                continue;
            }
            frameRequests.emplace(hint.GetSemanticFrameName(),
                                  NMegamind::TTypedSemanticFrameRequestDirectiveModel{action.GetSemanticFrame()});
        }
        directive.AddActionSpace(spaceId, std::move(frameRequests));
    }
    return directive;
}

NMegamind::TAddExternalEntitiesDescriptionDirectiveModel
MakeAddExternalEntitiesDescriptionDirective(const google::protobuf::RepeatedPtrField<NData::TExternalEntityDescription>& externalEntitiesDescription) {
    NMegamind::TAddExternalEntitiesDescriptionDirectiveModel directive{};
    for (const auto& entity : externalEntitiesDescription) {
        directive.AddExternalEntityDescription(entity);
    }
    return directive;
}

NMegamind::TAddConditionalActionsDirectiveModel
MakeAddConditionalActionsDirective(const google::protobuf::Map<TString, TConditionalAction>& conditionalActions) {
    NMegamind::TAddConditionalActionsDirectiveModel directive{};
    for (const auto& [conditionalActionId, action] : conditionalActions) {
        directive.AddConditionalAction(conditionalActionId, action);
    }
    return directive;
}

// FIXME(sparkle, zhigan, g-kostin): remove method below after HOLLYWOOD-586 supported in search app
TMaybe<TString> TryGetFillCloudUiDirectiveText(const NScenarios::TLayout& layout) {
    const auto* fillCloudUiDirective = FindIfPtr(layout.GetDirectives(), [](const NScenarios::TDirective& directive) {
        return directive.GetDirectiveCase() == NScenarios::TDirective::kFillCloudUiDirective;
    });
    if (fillCloudUiDirective) {
        return fillCloudUiDirective->GetFillCloudUiDirective().GetText();
    }
    return Nothing();
}

} // namespace

NMegamindAppHost::TScenarioTextResponse ParseTextResponse(const NScenarios::TScenarioResponseBody& scenarioResponseBody,
                                                          const TScenarioResponse& response) {
    NMegamindAppHost::TScenarioTextResponse textResponse;
    const auto& name = response.GetScenarioName();
    textResponse.SetScenarioName(name);
    for (const auto& card : scenarioResponseBody.GetLayout().GetCards()) {
        switch (card.GetCardCase()) {
            case NScenarios::TLayout_TCard::CardCase::kText:
                textResponse.AddText(card.GetText());
                break;
            case NScenarios::TLayout_TCard::CardCase::kTextWithButtons:
                textResponse.AddText(card.GetTextWithButtons().GetText());
                break;
            default:
                break;
        }
    }
    return textResponse;
}

// TProtocolScenarioWrapper ----------------------------------------------------
TProtocolScenarioWrapper::TProtocolScenarioWrapper(const TConfigBasedProtocolScenario& scenario,
                                                   const IContext& ctx,
                                                   const TSemanticFrames& semanticFrames,
                                                   const NMegamind::IGuidGenerator& guidGenerator,
                                                   EDeferredApplyMode deferApply,
                                                   bool restoreAllFromSession,
                                                   NMegamind::TItemProxyAdapter& itemProxyAdapter,
                                                   bool passDataSourcesInRequest)
    : TEffectfulScenarioWrapperBase(scenario, ctx, semanticFrames, guidGenerator, deferApply, restoreAllFromSession)
    , PassDataSourcesInRequest(passDataSourcesInRequest)
    , ItemProxyAdapter(itemProxyAdapter)
{
    const ISession* session = ctx.Session();
    if (restoreAllFromSession && session && session->GetPreviousScenarioName() == scenario.GetName()) {
        if (const auto protocolInfo = session->GetProtocolInfo(); protocolInfo.Defined()) {
            ProtocolInfo = *protocolInfo;
            switch (ProtocolInfo.GetArgumentsCase()) {
                case TSessionProto_TProtocolInfo::kApplyArguments:
                    Type = EApplyType::Apply;
                    break;
                case TSessionProto_TProtocolInfo::kCommitCandidate:
                    Type = EApplyType::Commit;
                    break;
                case TSessionProto_TProtocolInfo::kContinueArguments:
                    Type = EApplyType::Continue;
                    break;
                case TSessionProto_TProtocolInfo::ARGUMENTS_NOT_SET:
                    Type = Nothing();
                    LOG_ERR(ctx.Logger()) << "Could not restore ApplyType for scenario " << scenario.GetName();
                    break;
            }
        }
    }
}

inline NMegamind::TScenarioSessionView TProtocolScenarioWrapper::CreateSessionView(
    const TLightScenarioEnv& env) const
{
    return {
        .ScenarioState = env.State,
        .IsNewSession = IsNewScenarioSession(env.Ctx.Session(), GetConcreteScenario().GetName()),
        .IsSessionReset = IsSessionReset(env)
    };
}

inline NMegamind::TMementoDataView TProtocolScenarioWrapper::CreateMementoDataView(
    const TLightScenarioEnv& env) const
{
    return NMegamind::TMementoDataView(env.Ctx.MementoData(), GetConcreteScenario().GetName(),
                                       env.Ctx.SpeechKitRequest()->GetRequest().GetDeviceState().GetDeviceId(),
                                       env.Ctx.ClientInfo().Uuid);
}

TVector<TSemanticFrame> MakeFramesForRequest(const TVector<TSemanticFrame>& requestFrames,
                                             const TVector<TSemanticFrame>& allParsedSemanticFrames,
                                             const TScenario& scenario) {
    TVector<TSemanticFrame> result{requestFrames};
    if (!scenario.AlwaysRecieveAllParsedSemanticFrames()) {
        return result;
    }

    auto acceptedFrames = scenario.GetAcceptedFrames();
    for (const auto& frame : allParsedSemanticFrames) {
        const bool isAccepted = Find(acceptedFrames, frame.GetName()) != acceptedFrames.end();
        const bool hasInResult = FindIfPtr(result, [&frame](const TSemanticFrame& resultFrame) {
            return resultFrame.GetName() == frame.GetName();
        });
        if (isAccepted && !hasInResult) {
            result.push_back(frame);
        }
    }
    return result;
}

TStatus TProtocolScenarioWrapper::InitImpl(const TRequest& requestModel, const IContext& ctx,
                                           NMegamind::IDataSources& dataSources) {
    auto env = GetEnv(requestModel, ctx);

    const auto& scenario = GetConcreteScenario();
    auto updatedFrames = MakeFramesForRequest(SemanticFrames, requestModel.GetAllParsedSemanticFrames(), scenario);
    const NScenarios::TScenarioRunRequest request = ConstructRunRequest(
        scenario.GetConfig(), requestModel, env.Ctx, dataSources, CreateSessionView(env),
        CreateMementoDataView(env),
        updatedFrames,
        PassDataSourcesInRequest
    );
    RunInput = request.GetInput();
    CreateScenarioAnalyticsInfoBuilder()->SetStageStartTime(TString{TAG_RUN}, Now());

    TAppHostItemNames itemNames{scenario.GetName(), /* itemRequestSuffix= */ {}, /* itemResponseSuffix= */ {}};
    ItemProxyAdapter.PutIntoContext(request.GetBaseRequest(), itemNames.BaseRequest);
    ItemProxyAdapter.PutIntoContext(request.GetInput(), itemNames.RequestInput);
    return scenario.StartRun(env.Ctx, request, ItemProxyAdapter);
}

void AddStartScenarioContinueFlag(const TStringBuf scenarioName, NMegamind::TItemProxyAdapter& itemProxyAdapter) {
    // For now, only HollywoodMusic continue needs this logic
    if (scenarioName == HOLLYWOOD_MUSIC_SCENARIO) {
        itemProxyAdapter.PutIntoContext(google::protobuf::StringValue{}, TStringBuilder{} <<"start_" << scenarioName << "_continue");
    }
}

TStatus TProtocolScenarioWrapper::StartHeavyContinueImpl(const TRequest& /*requestModel*/, const IContext& /*ctx*/) {
    if (Mode != TScenario::EApplyMode::Continue) {
        return Success();
    }
    AddStartScenarioContinueFlag(GetScenario().GetName(), ItemProxyAdapter);
    return Success();
}

TStatus TProtocolScenarioWrapper::FinishContinueImpl(const TRequest& request, const IContext& ctx,
                                                     TScenarioResponse& scenarioResponse) {
    if (Mode != TScenario::EApplyMode::Continue) {
        return Success();
    }

    const auto& scenario = GetConcreteScenario();
    auto env = GetEnv(request, ctx);

    if (scenarioResponse.ContinueResponseIfExists()) {
        LOG_INFO(ctx.Logger()) << "Continue response already exists";
    } else if (ContinueHandle && ContinueHandle->Error()) {
        return *ContinueHandle->Error();
    } else {
        TErrorOr<NScenarios::TScenarioContinueResponse> status =
            scenario.FinishContinue(env.Ctx, ItemProxyAdapter);
        if (auto* e = status.Error()) {
            return std::move(*e);
        }
        scenarioResponse.SetContinueResponse(status.Value());
    }

    const auto* response = scenarioResponse.ContinueResponseIfExists();
    if (!response) {
        return TError{TError::EType::Logic} << "Continue response is not presented after finish continue";
    }

    LOG_DEBUG(env.Ctx.Logger()) << LogScenarioResponse(
        scenario.GetName(), *response, !env.Ctx.HasExpFlag(EXP_DEBUG_SHOW_SENSITIVE_DATA));

    auto error = OnContinueResponse(env.Ctx, *response, scenarioResponse);
    if (!error) {
        LOG_INFO(ctx.Logger()) << scenario.GetName() << " has successfully completed its continue stage";
    }

    return error;
}

TStatus TProtocolScenarioWrapper::FinalizeImpl(const TRequest& request, const IContext& ctx,
                                               TScenarioResponse& scenarioResponse) {
    const auto& serializerScenarioName = GetSerializerScenarioName(ctx);
    LOG_INFO(ctx.Logger()) << "Serializer scenario name: " << serializerScenarioName;

    auto& builder =
        scenarioResponse.ForceBuilder(ctx.SpeechKitRequest(), request, GuidGenerator, serializerScenarioName);

    if (request.GetRequestSource() == NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext) {
        builder.SetStackEngineParentRequestId(NMegamind::TStackEngine(request.GetStackEngineCore()).GetSessionId());
    }

    const auto* responseBody = scenarioResponse.ResponseBodyIfExists();
    if (!responseBody) {
        return Success();
    }

    return BuildResponse(request, ctx, *responseBody, State, /* version= */ VERSION_STRING, scenarioResponse);
}

TStatus TProtocolScenarioWrapper::AskImpl(const TRequest& request, const IContext& ctx,
                                          TScenarioResponse& scenarioResponse) {
    using EApplyMode = TScenario::EApplyMode;
    using TScenarioRunResponse = NScenarios::TScenarioRunResponse;

    scenarioResponse.SetScenarioType(TScenarioResponse::EScenarioType::Protocol);

    TErrorOr<TScenarioRunResponse> status = GetConcreteScenario().FinishRun(ctx, ItemProxyAdapter);

    if (auto* e = status.Error()) {
        return std::move(*e);
    }

    const auto& response = status.Value();

    LOG_WITH_TYPE(ctx.Logger(), TLOG_DEBUG, ELogMessageType::MegamindRetrieveScenarioRunResponses)
        << LogScenarioResponse(GetScenario().GetName(), response, !ctx.HasExpFlag(EXP_DEBUG_SHOW_SENSITIVE_DATA));

    ProtocolInfo.Clear();

    EnrichProtocolInfo(ProtocolInfo, request);

    if (response.HasFeatures()) {
        const auto& features = response.GetFeatures();
        Features.MutableScenarioFeatures()->CopyFrom(features);
    }

    if (response.HasUserInfo()) {
        UserInfoBuilder.CopyFrom(response.GetUserInfo());
    }
    switch (response.GetResponseCase()) {
        case TScenarioRunResponse::ResponseCase::kResponseBody:
            Mode = EApplyMode::Skip;
            return OnResponseBody(ctx, response.GetResponseBody(), response.GetVersion(), scenarioResponse);
        case TScenarioRunResponse::ResponseCase::kCommitCandidate:
            Mode = EApplyMode::Call;
            Type = EApplyType::Commit;
            *ProtocolInfo.MutableCommitCandidate() = response.GetCommitCandidate();
            break;
        case TScenarioRunResponse::ResponseCase::kApplyArguments:
            Mode = EApplyMode::Call;
            Type = EApplyType::Apply;
            *ProtocolInfo.MutableApplyArguments() = response.GetApplyArguments();
            break;
        case TScenarioRunResponse::ResponseCase::kContinueArguments:
            Type = EApplyType::Continue;
            Mode = EApplyMode::Continue;
            *ProtocolInfo.MutableContinueArguments() = response.GetContinueArguments();
            break;
        case TScenarioRunResponse::ResponseCase::kError:
            Mode = EApplyMode::Skip;
            Y_ASSERT(response.HasError());
            return TError{TError::EType::ScenarioError} << response.GetError().GetMessage();
        case TScenarioRunResponse::ResponseCase::RESPONSE_NOT_SET:
            Mode = EApplyMode::Skip;
            return ResponseNotSetErrorFor(GetScenario().GetName(), "Run");
    }
    return Success();
}

TStatus TProtocolScenarioWrapper::CallApply(const TRequest& request, const IContext& ctx) {
    if (!Type.Defined()) {
        return TError{TError::EType::Logic} << "Undefined apply type";
    }
    auto env = GetApplyEnv(request, ctx);
    const auto& scenario = GetConcreteScenario();
    const auto& sessionView = CreateSessionView(env);
    auto updatedFrames = MakeFramesForRequest(SemanticFrames, request.GetAllParsedSemanticFrames(), scenario);
    switch (*Type) {
        case EApplyType::Commit: {
            return scenario.StartCommit(
                env.Ctx,
                NMegamind::ConstructApplyRequest(scenario.GetConfig(), request, env.Ctx, sessionView,
                                                 CreateMementoDataView(env), updatedFrames,
                                                 ProtocolInfo.GetCommitCandidate().GetArguments()),
                ItemProxyAdapter);
        }
        case EApplyType::Apply: {
            return scenario.StartApply(
                env.Ctx,
                NMegamind::ConstructApplyRequest(scenario.GetConfig(), request, env.Ctx, sessionView,
                                                 CreateMementoDataView(env), updatedFrames,
                                                 ProtocolInfo.GetApplyArguments()),
                ItemProxyAdapter);
        }
        case EApplyType::Continue: {
            return Success(); // Pass.
        }
    }

    return Success();
}

TStatus TProtocolScenarioWrapper::GetApplyResponse(const TRequest& request, const IContext& ctx, TScenarioResponse& scenarioResponse) {
    if (!Type.Defined()) {
        return TError{TError::EType::Logic} << "Undefined apply type";
    }
    auto env = GetApplyEnv(request, ctx);
    const auto& scenario = GetConcreteScenario();
    switch (*Type) {
        case EApplyType::Commit: {
            TErrorOr<NScenarios::TScenarioCommitResponse> status = scenario.FinishCommit(env.Ctx, ItemProxyAdapter);
            if (auto* e = status.Error()) {
                return std::move(*e);
            }
            const auto& response = status.Value();
            LOG_INFO(env.Ctx.Logger()) << LogScenarioResponse(
                scenario.GetName(), response, !env.Ctx.HasExpFlag(EXP_DEBUG_SHOW_SENSITIVE_DATA));
            return OnCommitResponse(env.Ctx, response, scenarioResponse);
        }
        case EApplyType::Apply: {
            TErrorOr<NScenarios::TScenarioApplyResponse> status = scenario.FinishApply(env.Ctx, ItemProxyAdapter);
            if (auto* e = status.Error()) {
                return std::move(*e);
            }

            const auto& response = status.Value();
            LOG_DEBUG(env.Ctx.Logger()) << LogScenarioResponse(
                scenario.GetName(), response, !env.Ctx.HasExpFlag(EXP_DEBUG_SHOW_SENSITIVE_DATA));
            return OnApplyResponse(env.Ctx, response, scenarioResponse);
        }
        case EApplyType::Continue: {
            return Success(); // Pass.
        }
    }

    return Success();
}

TStatus TProtocolScenarioWrapper::OnCommitResponse(const IContext& ctx,
                                                   const NScenarios::TScenarioCommitResponse& scenarioResponse,
                                                   TScenarioResponse& response) {
    using NScenarios::TScenarioCommitResponse;
    switch (scenarioResponse.Response_case()) {
        case TScenarioCommitResponse::ResponseCase::kSuccess: {
            return OnResponseBody(ctx, ProtocolInfo.GetCommitCandidate().GetResponseBody(), scenarioResponse.GetVersion(),
                           response);
        }
        case TScenarioCommitResponse::ResponseCase::kError: {
            return OnError(scenarioResponse.GetError(), response);
        }
        case TScenarioCommitResponse::ResponseCase::RESPONSE_NOT_SET: {
            return ResponseNotSetErrorFor(GetScenario().GetName(), "Commit");
        }
    }
}

template <typename TProtoResponse>
TStatus TProtocolScenarioWrapper::OnFinalResponse(const IContext& ctx, const TProtoResponse& scenarioResponse,
                                                  TScenarioResponse& response, const TString& method) {
    switch (scenarioResponse.Response_case()) {
        case TProtoResponse::ResponseCase::kResponseBody: {
            return OnResponseBody(ctx, scenarioResponse.GetResponseBody(), scenarioResponse.GetVersion(),
                           response);
        }
        case TProtoResponse::ResponseCase::kError: {
            return OnError(scenarioResponse.GetError(), response);
        }
        case TProtoResponse::ResponseCase::RESPONSE_NOT_SET: {
            return ResponseNotSetErrorFor(GetScenario().GetName(), method);
        }
    }
}

TStatus TProtocolScenarioWrapper::OnApplyResponse(const IContext& ctx,
                                                  const NScenarios::TScenarioApplyResponse& scenarioResponse,
                                                  TScenarioResponse& response) {
    return OnFinalResponse(ctx, scenarioResponse, response, "Apply");
}

TStatus TProtocolScenarioWrapper::OnContinueResponse(const IContext& ctx,
                                                     const NScenarios::TScenarioContinueResponse& scenarioResponse,
                                                     TScenarioResponse& response) {
    return OnFinalResponse(ctx, scenarioResponse, response, "Continue");
}

TStatus TProtocolScenarioWrapper::OnResponseBody(const IContext& ctx,
                                                 const TScenarioResponseBody& scenarioResponseBody,
                                                 const TString& version, TScenarioResponse& response) {
    if (auto error = CheckScenarioVersion(version, ctx); error.Defined()) {
        return std::move(error);
    }
    response.SetResponseBody(scenarioResponseBody);
    return Success();
}

TStatus TProtocolScenarioWrapper::CheckScenarioVersion(const TString& version, const IContext& ctx) const {
    if (version != VERSION_STRING) {
        if (GetScenario().GetName() == MM_PROTO_VINS_SCENARIO) {
            LOG_ERROR(ctx.Logger()) << GetScenario().GetName() << "'s scenario version differs from Megamind's one, "
                                    << version << " and " << VERSION_STRING << " respectively.";
        }
        if (ctx.HasExpFlag(EXP_PREFIX_FAIL_ON_SCENARIO_VERSION_MISMATCH + GetScenario().GetName())) {
            return TError{TError::EType::VersionMismatch} << "Scenario's version mismatch";
        }
    }
    return Success();
}

void BuildScenarioResponseFromResponseBody(const NScenarios::TScenarioResponseBody& scenarioResponseBody,
                                           TScenarioResponse& response, const IContext& ctx, const TRequest& request,
                                           const NMegamind::IGuidGenerator& guidGenerator,
                                           NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                           NMegamind::TAnalyticsInfoBuilder& analyticsInfoBuilder,
                                           const TString& version,
                                           const NScenarios::TScenarioBaseRequest::ERequestSourceType requestSourceType,
                                           const TMaybe<TDirectiveChannel::EDirectiveChannel> ttsPlayPlaceHolderChannel,
                                           const TMaybe<bool> forcedShouldListen,
                                           const TMaybe<TString> forcedEmotion) {
    auto& builder = response.ForceBuilder(ctx.SpeechKitRequest(), request, guidGenerator);

    // Should be the first added directive https://st.yandex-team.ru/MEGAMIND-3361
    if (!scenarioResponseBody.GetActionSpaces().empty()) {
        builder.AddFrontDirective(MakeUpdateSpaceActionDirective(scenarioResponseBody.GetActionSpaces(), ctx.Logger()));
    }
    if (!scenarioResponseBody.GetConditionalActions().empty()) {
        builder.AddFrontDirective(MakeAddConditionalActionsDirective(scenarioResponseBody.GetConditionalActions()));
    }
    if (!scenarioResponseBody.GetExternalEntitiesDescription().empty()) {
        builder.AddFrontDirective(MakeAddExternalEntitiesDescriptionDirective(scenarioResponseBody.GetExternalEntitiesDescription()));
    }

    builder.SetProductScenarioName(scenarioResponseBody.GetAnalyticsInfo().GetProductScenarioName());
    {
        auto stackEngine = std::make_unique<NMegamind::TStackEngine>(request.GetStackEngineCore());
        if (scenarioResponseBody.HasStackEngine()) {
            ProcessStackEngine(*stackEngine, response.GetScenarioName(), scenarioResponseBody.GetStackEngine(),
                               ctx.SpeechKitRequest().RequestId(),
                               scenarioResponseBody.GetAnalyticsInfo().GetProductScenarioName());
        }
        builder.SetStackEngine(std::move(stackEngine));
        if (ctx.HasExpFlag(EXP_ENABLE_STACK_ENGINE_MEMENTO_BACKUP)) {
            builder.AddDirectiveToVoiceResponse(
                NMegamind::TMementoChangeUserObjectsDirectiveModel(
                    MM_STACK_ENGINE_MEMENTO_KEY,
                    builder.GetStackEngine()->GetCore()
                ));
        }
    }

    const auto& layout = scenarioResponseBody.GetLayout();
    NMegamind::TScenarioProtoDeserializer deserializer{builder.GetSerializerMeta(),
                                                    ctx.Logger(),
                                                    ParseProtoMap(scenarioResponseBody.GetFrameActions()),
                                                    ParseProtoMap(scenarioResponseBody.GetActionSpaces()),
                                                    TryGetFillCloudUiDirectiveText(layout)};

    if (scenarioResponseBody.GetResponseCase() == NScenarios::TScenarioResponseBody::kGrpcResponse) {
        //NScenarios::TServerDirective directive{};
        //auto& grpcResponseDirective = *directive.MutableGProxyGrpcResponseDirective();
        //grpcResponseDirective.MutablePayload()->CopyFrom(scenarioResponseBody.GetGrpcResponse().GetPayload());

        auto payload = TProtoStructBuilder{}
                .Set("payload", MessageToStruct(scenarioResponseBody.GetGrpcResponse().GetPayload()))
                .Build();

        auto directiveModel = MakeIntrusive<NMegamind::TUniversalUniproxyDirectiveModel>("grpc_response", payload);
        builder.AddDirectiveToVoiceResponse(*directiveModel);

    } else {
        if (layout.HasContentProperties()) {
            builder.SetContentProperties(layout.GetContentProperties());
        } else if (layout.GetContainsSensitiveData()) {
            TContentProperties contentProperties{};
            contentProperties.SetContainsSensitiveDataInRequest(true);
            contentProperties.SetContainsSensitiveDataInResponse(true);
            builder.SetContentProperties(contentProperties);
        }

        {
            auto templates = layout.GetDiv2Templates();
            deserializer.CreateDeepLinks(templates);
            builder.SetDiv2Templates(std::move(templates));
        }
        for (const auto& card : layout.GetCards()) {
            if (auto cardModel = deserializer.Deserialize(card); cardModel) {
                builder.AddCard(*cardModel);
            } else {
                LOG_WARN(ctx.Logger()) << "Unable to deserialize provided card from " << response.GetScenarioName();
            }
        }

        auto addTtsPlayPlaceHolder = [&builder, &deserializer] (const TString& name,
                                                                const TDirectiveChannel::EDirectiveChannel channel) {
            NScenarios::TDirective directive;
            auto& ttsPlayPlaceHolderDirective = *directive.MutableTtsPlayPlaceholderDirective();
            ttsPlayPlaceHolderDirective.SetName(name);
            ttsPlayPlaceHolderDirective.SetDirectiveChannel(channel);
            if (auto model = deserializer.Deserialize(directive); model) {
                builder.AddDirective(*model);
            }
        };

        if (requestSourceType == NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext && ttsPlayPlaceHolderChannel.Defined() &&
            !FindIfPtr(layout.GetDirectives(), [] (const NScenarios::TDirective& directive) {
                return directive.GetDirectiveCase() == NScenarios::TDirective::kTtsPlayPlaceholderDirective;
            }))
        {
            addTtsPlayPlaceHolder("tts_play_placeholder", *ttsPlayPlaceHolderChannel);
        }
        for (const auto& directive : layout.GetDirectives()) {
            if (auto directiveModel = deserializer.Deserialize(directive); directiveModel) {
                const auto directiveCase = directive.GetDirectiveCase();
                if (VOICE_RESPONSE_DIRECTIVES.contains(directiveCase)) {
                    builder.AddDirectiveToVoiceResponse(*directiveModel);
                } else if (directiveCase == NScenarios::TDirective::kTtsPlayPlaceholderDirective && ttsPlayPlaceHolderChannel.Defined() &&
                           requestSourceType == NScenarios::TScenarioBaseRequest_ERequestSourceType_GetNext)
                {
                    addTtsPlayPlaceHolder(directive.GetTtsPlayPlaceholderDirective().GetName(), *ttsPlayPlaceHolderChannel);
                } else {
                    builder.AddDirective(*directiveModel);
                }
                if (directiveCase == NScenarios::TDirective::kFindContactsDirective &&
                    directive.GetFindContactsDirective().GetAddAsrContactBook()) {
                    builder.AddDirective(NMegamind::TAddContactBookAsrDirectiveModel());
                }
            } else {
                LOG_WARN(ctx.Logger()) << "Unable to deserialize provided directive from " << response.GetScenarioName();
            }
        }

        // TODO(alkapov): remove after 51 release
        if (layout.GetSuggestButtons().empty()) {
            for (const auto& suggest : layout.GetSuggests()) {
                if (auto suggestModel = deserializer.Deserialize(suggest, /* fromSuggest= */ true); suggestModel) {
                    builder.AddSuggest(*suggestModel);
                } else {
                    LOG_WARN(ctx.Logger()) << "Unable to deserialize provided suggest from " << response.GetScenarioName();
                }
            }
        }

        for (const auto& suggest : layout.GetSuggestButtons()) {
            if (const auto suggestModel = deserializer.Deserialize(suggest); suggestModel) {
                builder.AddSuggest(*suggestModel);
            } else {
                LOG_WARN(ctx.Logger()) << "Unable to deserialize provided suggest from " << response.GetScenarioName();
            }
        }

        if (const auto& speech = layout.GetOutputSpeech()) {
            builder.SetOutputSpeech(speech);
        }

        builder.ShouldListen(layout.GetShouldListen());
    }

    for (const auto& directive : scenarioResponseBody.GetServerDirectives()) {
        if (auto directiveModel = deserializer.Deserialize(directive); directiveModel) {
            if (directiveModel->GetType() == NMegamind::EDirectiveType::ProtobufUniproxyAction) {
                builder.AddProtobufUniproxyDirective(*directiveModel);
            } else {
                builder.AddDirectiveToVoiceResponse(*directiveModel);
            }
        } else {
            LOG_WARN(ctx.Logger()) << "Unable to deserialize provided server directive from " << response.GetScenarioName();
        }
    }

    if (const auto& name = megamindAnalyticsInfo.GetParentProductScenarioName()) {
        analyticsInfoBuilder.SetParentProductScenarioName(name);
    }

    if (const auto* stackEngine = builder.GetStackEngine();
        stackEngine && stackEngine->IsUpdated())
    {
        const auto& sessionId = stackEngine->GetSessionId();
        analyticsInfoBuilder.SetParentRequestId(sessionId);
        builder.SetStackEngineParentRequestId(sessionId);

        analyticsInfoBuilder.SetParentProductScenarioName(stackEngine->GetProductScenarioName());
        if (!stackEngine->IsEmpty()) {
            builder.AddDirective(NImpl::CreateGetNextDirectiveFromStackEngine(*stackEngine, deserializer));
        }
    }

    if (forcedShouldListen.Defined()) {
        builder.ShouldListen(forcedShouldListen.GetRef());
    }

    if (forcedEmotion.Defined()) {
        builder.SetOutputEmotion(forcedEmotion.GetRef());
    }

    builder.SetDirectivesExecutionPolicy(layout.GetDirectivesExecutionPolicy());

    analyticsInfoBuilder.SetVersion(version);
    if (scenarioResponseBody.HasAnalyticsInfo()) {
        analyticsInfoBuilder.Update(scenarioResponseBody.GetAnalyticsInfo());
    }
    if (scenarioResponseBody.HasSemanticFrame()) {
        builder.PutSemanticFrame(scenarioResponseBody.GetSemanticFrame());
        analyticsInfoBuilder.SetSemanticFrame(scenarioResponseBody.GetSemanticFrame());
    }

    const auto& actions = scenarioResponseBody.GetFrameActions();
    if (!actions.empty()) {
        builder.PutActions(actions);
        analyticsInfoBuilder.SetFrameActions(actions);
    }
    if (scenarioResponseBody.GetEntities().empty()) {
        builder.PutEntities(scenarioResponseBody.GetEntities());
    }

    builder.SetLayout(std::make_unique<NScenarios::TLayout>(std::move(scenarioResponseBody.GetLayout())));

    if (scenarioResponseBody.HasResponseErrorMessage()) {
        builder.SetResponseErrorMessage(scenarioResponseBody.GetResponseErrorMessage());
    }

}

TStatus TProtocolScenarioWrapper::BuildResponse(const TRequest& request, const IContext& ctx,
                                                const NScenarios::TScenarioResponseBody& scenarioResponseBody,
                                                TState& state, const TString& version, TScenarioResponse& response) {
    *state.MutableState() = scenarioResponseBody.GetState();
    if (const auto textResponse = ParseTextResponse(scenarioResponseBody, response); !textResponse.GetText().empty()) {
        ItemProxyAdapter.PutIntoContext(textResponse, NMegamind::AH_ITEM_SCENARIOS_RESPONSE_MONITORING);
    }

    BuildScenarioResponseFromResponseBody(scenarioResponseBody, response, ctx, request,
                                          GuidGenerator, MegamindAnalyticsInfo, AnalyticsInfoBuilder, version,
                                          ProtocolInfo.GetRequestSourceType(),
                                          ProtocolInfo.HasChannel()
                                            ? TMaybe<TDirectiveChannel::EDirectiveChannel>(ProtocolInfo.GetChannel())
                                            : Nothing(),
                                          ProtocolInfo.HasForcedShouldListen()
                                            ? TMaybe<bool>(ProtocolInfo.GetForcedShouldListen().value())
                                            : Nothing(),
                                          ProtocolInfo.HasForcedEmotion()
                                            ? TMaybe<TString>(ProtocolInfo.GetForcedEmotion())
                                            : Nothing());

    ProtocolInfo.SetRequestIsExpected(scenarioResponseBody.GetExpectsRequest());

    return Success();
}

TStatus TProtocolScenarioWrapper::OnError(const NScenarios::TScenarioError& error, TScenarioResponse& /*response*/) {
    return TError{TError::EType::ScenarioError} << "scenario " << GetScenario().GetName() << " failed with \"" << error.GetMessage() << "\".";
}

const TString& TProtocolScenarioWrapper::GetSerializerScenarioName(const IContext& ctx) const {
    auto* session = ctx.Session();
    const auto& scenarioName = GetScenario().GetName();
    return session && ctx.ScenarioConfig(scenarioName).GetReplaceScenarioNameWithPrevious()
           ? session->GetPreviousScenarioName()
           : scenarioName;
}


// TAppHostProxyProtocolScenarioWrapper ----------------------------------------
TAppHostProxyProtocolScenarioWrapper::TAppHostProxyProtocolScenarioWrapper(
    const TConfigBasedAppHostProxyProtocolScenario& scenario,
    const IContext& ctx,
    const TSemanticFrames& semanticFrames,
    const NMegamind::IGuidGenerator& guidGenerator,
    EDeferredApplyMode deferApply,
    bool restoreAllFromSession,
    NMegamind::TItemProxyAdapter& itemProxyAdapter)
    : TProtocolScenarioWrapper(scenario, ctx, semanticFrames, guidGenerator, deferApply, restoreAllFromSession,
                               itemProxyAdapter, scenario.PassDataSourcesInContext(ctx))
    , ScenarioCopy(scenario)
{
}

void TAppHostProxyProtocolScenarioWrapper::Accept(const IScenarioVisitor& visitor) const {
    visitor.Visit(ScenarioCopy);
}


// TAppHostPureProtocolScenarioWrapper ----------------------------------------
TAppHostPureProtocolScenarioWrapper::TAppHostPureProtocolScenarioWrapper(
    const TConfigBasedAppHostPureProtocolScenario& scenario,
    const IContext& ctx,
    const TSemanticFrames& semanticFrames,
    const NMegamind::IGuidGenerator& guidGenerator,
    EDeferredApplyMode deferApply,
    bool restoreAllFromSession,
    NMegamind::TItemProxyAdapter& itemProxyAdapter)
    : TProtocolScenarioWrapper(scenario, ctx, semanticFrames, guidGenerator, deferApply, restoreAllFromSession,
                               itemProxyAdapter,  /* passDataSourcesInRequest= */ false)
    , ScenarioCopy(scenario)
{
}

void TAppHostPureProtocolScenarioWrapper::Accept(const IScenarioVisitor& visitor) const {
    visitor.Visit(ScenarioCopy);
}

} // namespace NAlice
