#include "vins.h"

#include <alice/bass/forms/user_aware_handler.h>

#include <alice/bass/forms/protocol_scenario/protocol_scenario_utils.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/metrics/metrics.h>
#include <alice/bass/libs/video_common/nlg_utils.h>

#include <alice/library/util/variant.h>

#include <google/protobuf/any.pb.h>
#include <google/protobuf/wrappers.pb.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/string/builder.h>

namespace NBASS {

namespace {

using namespace NAlice;
using namespace NAlice::NScenarios;

constexpr TStringBuf FORM_SIGNAL_PREFIX = "form_";
constexpr TStringBuf ACTION_SIGNAL_PREFIX = "action_";
constexpr TStringBuf SIGNAL_SUFFIX = "_responseTime";
constexpr TStringBuf SIGNAL_SLA_SUFFIX = "_SLA";

NMonitoring::TLabels BuildScenariosLabels(TContext& ctx, TStringBuf result) {
    NMonitoring::TLabels labels;
    labels.Add("form", ctx.FormName());
    labels.Add("client", ctx.MetaClientInfo().Name);
    labels.Add("result", result);
    return labels;
}

/** return true if the error calback was called, false otherwise */
template <typename TOnError, typename TOnBody>
bool MakeResponseBodyOrError(TOnError&& onError, TOnBody&& onBody, const IContinuation& cont) {
    Y_ENSURE(!cont.NeedsApply());
    if (cont.GetResult()) {
        onError(cont.GetResult()->Msg);
        return true;
    }

    TContext& ctx = cont.GetContext();
    auto protoOrError =
        BassContextToProtocolResponseBody(ctx, NBASS::NVideoCommon::NlgTemplateIdByMegamindIntent(ctx.FormName()));
    auto visitor = MakeLambdaVisitor(
        [&onError](const NAlice::TError& error) {
            onError(error.ErrorMsg);
            return true;
        },
        [&onBody](const TScenarioResponseBody& response) {
            onBody(response);
            return false;
        });
    return Visit(visitor, protoOrError);
}

template <typename TProto>
bool MakeResponseBodyOrError(TProto& proto, const IContinuation& cont) {
    return MakeResponseBodyOrError(
        [&proto](const TString& error) { proto.MutableError()->SetMessage(error); } /* onError */,
        [&proto](const TScenarioResponseBody& body) { *proto.MutableResponseBody() = body; }, cont) /* onBody */;
}


template <typename TProto>
void MakeApplyArguments(TProto& proto, const IContinuation& cont) {
    google::protobuf::StringValue applyArguments;
    *applyArguments.mutable_value() = cont.ToJson().ToJson();
    proto.PackFrom(applyArguments);
}

const TString BASS_VERSION = TStringBuilder() << GetArcadiaSourceUrl() << '@' << GetArcadiaLastChange();

} // namespace

// IContinuation ---------------------------------------------------------------
NSc::TValue IContinuation::ToJson() const {
    NSc::TValue result;
    result["IsFinished"].SetBool(IsFinished());
    result["ObjectTypeName"] = GetName();

    if (IsFinished()) {
        result["State"].SetString(GetContext().ToJson().ToJson());
        if (TResultValue res = GetResult())
            res->ToJson(result);

    } else {
        result["State"] = ToJsonImpl().ToJson();
    }

    return result;
}

TScenarioRunResponse IContinuation::AsProtocolRunResponse() const {
    TScenarioRunResponse response;

    switch (FinishStatus) {
        case EFinishStatus::Completed:
            MakeResponseBodyOrError(response, *this);
            break;

        case EFinishStatus::NeedApply:
            MakeApplyArguments(*response.MutableApplyArguments(), *this);
            break;

        case EFinishStatus::NeedCommit:
            if (!MakeResponseBodyOrError(
                    [&response](const TString& error) { response.MutableError()->SetMessage(error); } /* onError */,
                    [&response](const TScenarioResponseBody& body) {
                        *response.MutableCommitCandidate()->MutableResponseBody() = body;
                    } /* onBody */,
                    *this)) {
                // We shouldn't set apply arguments if error is being returned.
                MakeApplyArguments(*response.MutableCommitCandidate()->MutableArguments(), *this);
            }
            break;
    }
    response.SetVersion(BASS_VERSION);
    return response;
}

TScenarioApplyResponse IContinuation::AsProtocolApplyResponse() const {
    Y_ENSURE(IsFinished());
    TScenarioApplyResponse response;
    MakeResponseBodyOrError(response, *this);
    response.SetVersion(BASS_VERSION);
    return response;
}

TScenarioCommitResponse IContinuation::AsProtocolCommitResponse() const {
    Y_ENSURE(IsFinished());
    TScenarioCommitResponse response;
    if (auto error = GetResult()) {
        response.MutableError()->SetMessage(std::move(error->Msg));
    } else {
        *response.MutableSuccess() = TScenarioCommitResponse::TSuccess{};
    }
    response.SetVersion(BASS_VERSION);
    return response;
}

// THollywoodContinuation ------------------------------------------------------

THollywoodContinuation::THollywoodContinuation(NSc::TValue value, TGlobalContextPtr globalContext,
                                               NSc::TValue meta, const TString& authHeader,
                                               const TMaybe<TString>& userTicketHeader,
                                               const NSc::TValue& configPatch)
    : IContinuation{IContinuation::EFinishStatus::NeedApply}
{
    auto state = NSc::TValue::FromJson(value["State"].GetString());

    const auto reqId = meta["request_id"];

    NSc::TValue contextValue = std::move(state["context"]);
    contextValue["meta"] = std::move(meta);

    TContext::TInitializer initData(globalContext, reqId, authHeader, /* appInfoHeader= */ {},
                                    /* fakeTimeHeader= */ {}, userTicketHeader, /* speechKitEvent= */ {});
    initData.ConfigPatch = configPatch;
    initData.Id = contextValue["form"]["name"];
    TResultValue contextParseResult = TContext::FromJson(contextValue, initData, &Context_);
    if (contextParseResult.Defined()) {
        ythrow yexception() << "Cannot deserialize context: " << contextParseResult->Msg;
    }

    ApplyArguments_ = std::move(state["apply_arguments"]);
    FeaturesData_ = std::move(state["features_data"]);
}

// IHandler --------------------------------------------------------------------
TResultValue IHandler::DoSetup(TSetupContext& /* ctx */) {
    return Nothing();
}

TResultValue TContinuableHandler::Do(TRequestHandler& r) {
    auto preset = Prepare(r);
    return preset->ApplyIfNotFinished();
}

TResultValue TDefaultHandler::Do(TRequestHandler& r) {
    return TError{
        TError::EType::SYSTEM,
        TStringBuilder{} << "No handler for the form '" << r.Ctx().FormName() << "' found"
    };
}


THandlersMap::THandlersMap(IGlobalContext& globalCtx)
    : GlobalCtx(globalCtx)
    , DefaultHandlerFactory([](){ return MakeHolder<TDefaultHandler>(); })
{
    NSc::TValue formsSLAsJson(NSc::TValue::FromJsonThrow(NResource::Find("forms_sla.json")));
    NSc::TValue actionsSLAJson(NSc::TValue::FromJsonThrow(NResource::Find("actions_sla.json")));

    for (const auto& t : formsSLAsJson.GetDict()) {
        FormSLAs.emplace(t.first, TDuration::MilliSeconds(t.second));
    }
    for (const auto& t : actionsSLAJson.GetDict()) {
        ActionSLAs.emplace(t.first, TDuration::MilliSeconds(t.second));
    }
}

void THandlersMap::RegisterFormHandler(TStringBuf name, THandlerFactory factory) {
    Y_ENSURE(FormSLAs.FindPtr(name), TStringBuilder() << "Please add SLA time for form " << name << " to alice/bass/data/forms_sla.json");
    const TConfig& config = GlobalCtx.Config();
    const TUserAwareHandler::TConfig userAwareHandlerConfig = {
        .NameDelay = config.PersonalizationNameDelay(),
        .BiometryScoreThreshold = config.PersonalizationBiometryScoreThreshold(),
        .PersonalizationDropProbabilty = config.PersonalizationDropProbabilty(),
        .PersonalizationAdditionalDataSyncTimeout = config.PersonalizationAdditionalDataSyncTimeout(),
        .MusicNamePronounceDelayPeroid = config.PersonalizationMusicNamePronounceDelayPeriod(),
        .MusicNamePronounceDelayCount = config.PersonalizationMusicNamePronounceDelayCount(),
        .MusicNamePronouncePeriod = config.PersonalizationMusicNamePronouncePeriod()
    };

    FormHandlers.emplace(name, [factory, userAwareHandlerConfig]()
        {
            return MakeHolder<TUserAwareHandler>(
                factory(),
                userAwareHandlerConfig,
                MakeHolder<TUserAwareHandler::TDelegate>(),
                MakeHolder<TBlackBoxAPI>(),
                MakeHolder<TDataSyncAPI>()
            );
        }
    );

    GlobalCtx.Counters().BassCounters().RegisterUnistatHistogram(TStringBuilder() << FORM_SIGNAL_PREFIX << name << SIGNAL_SUFFIX);
    GlobalCtx.Counters().BassCounters().RegisterUnistatHistogram(TStringBuilder() << FORM_SIGNAL_PREFIX << name << SIGNAL_SUFFIX << SIGNAL_SLA_SUFFIX);
}

void THandlersMap::RegisterActionHandler(TStringBuf name, THandlerFactory factory) {
    Y_ENSURE(ActionSLAs.FindPtr(name), TStringBuilder() << "Please add SLA time for action " << name << " to alice/bass/data/actions_sla.json");
    ActionHandlers.emplace(name, factory);
    GlobalCtx.Counters().BassCounters().RegisterUnistatHistogram(TStringBuilder() << ACTION_SIGNAL_PREFIX << name << SIGNAL_SUFFIX);
    GlobalCtx.Counters().BassCounters().RegisterUnistatHistogram(TStringBuilder() << ACTION_SIGNAL_PREFIX << name << SIGNAL_SUFFIX << SIGNAL_SLA_SUFFIX);
}

void THandlersMap::RegisterContinuableHandler(TStringBuf name, TContinuableHandlerFactory factory) {
    ContinuableHandlers.emplace(name, factory);
}

void THandlersMap::RegisterFormAndContinuableHandler(TStringBuf name, TContinuableHandlerFactory factory) {
    RegisterFormHandler(name, factory);
    RegisterContinuableHandler(name, factory);
}

const THandlersMap::THandlerFactory* THandlersMap::ActionHandler(TStringBuf name) const {
    return ActionHandlers.FindPtr(name);
}

const THandlersMap::TContinuableHandlerFactory* THandlersMap::ContinuableHandler(TStringBuf name) const {
    return ContinuableHandlers.FindPtr(name);
}

const THandlersMap::THandlerFactory* THandlersMap::FormHandler(TStringBuf formName) const {
    auto lowerBound = FormHandlers.lower_bound(formName);
    if (lowerBound == FormHandlers.end())
        return &DefaultHandlerFactory;

    TStringBuf handlerName = lowerBound->first;
    // exact match, it is unique map so there is no sense to check upper bound.
    if (formName == handlerName)
        return &lowerBound->second;

    // Lower bound gives greater element if exact match is not found.
    if (lowerBound != FormHandlers.begin()) {
        --lowerBound;
        handlerName = lowerBound->first;
    } else {
        return &DefaultHandlerFactory;
    }

    // Allow '*' for suffix only.
    if (handlerName.back() == '*' &&
        formName.find(handlerName.substr(0, handlerName.size() - 1)) == 0)
        return &lowerBound->second;

    return &DefaultHandlerFactory;
}

const TDuration THandlersMap::FormSLA(TStringBuf name) const {
    if (auto ptr = FormSLAs.FindPtr(name); ptr) {
        return *ptr;
    }
    return {};
}

const TDuration THandlersMap::ActionSLA(TStringBuf name) const {
    if (auto ptr = ActionSLAs.FindPtr(name); ptr) {
        return *ptr;
    }
    return {};
}

TResultValue TRequestHandler::RunFormHandler() {
    try {
        const TStringBuf formName{Ctx().FormName()};
        const THandlerFactory* handlerFactory = Context->GlobalCtx().FormHandler(formName);
        if (!handlerFactory) {
            return TError(TError::EType::SYSTEM,
                          TStringBuilder() << "No handler for the form '" << formName << "' found");
        }

        THolder<IHandler> handler = (*handlerFactory)();
        if (!handler) {
            return TError(TError::EType::SYSTEM, "Unable to create handler");
        }

        const TInstant startTime = TInstant::Now();

        TResultValue result;
        try {
            result = handler->Do(*this);
        } catch (TErrorException& e) {
            result = e.Error();
        } catch (yexception& e) {
            result = TError(
                TError::EType::SYSTEM,
                TStringBuilder() << "Caught exception: " << e.what()
            );
        }

        TString signalName = TStringBuilder() << FORM_SIGNAL_PREFIX << formName << SIGNAL_SUFFIX;
        auto duration = TInstant::Now() - startTime;
        const auto* signal = Context->GlobalCtx().Counters().BassCounters().UnistatHistograms.FindPtr(signalName);
        if (signal) {
            (*signal)->PushSignal(duration.MilliSeconds());
        }
        const auto* signalSLA = Context->GlobalCtx().Counters().BassCounters().UnistatHistograms.FindPtr(signalName + SIGNAL_SLA_SUFFIX);
        if (signalSLA) {
            (*signalSLA)->PushSignal((duration - Context->GlobalCtx().FormSLA(formName)).MilliSeconds());
        }
        TStringBuf resultText = result ? TStringBuf("fail") : TStringBuf("success");
        Ctx().GlobalCtx().Counters().Sensors().Rate(BuildScenariosLabels(Ctx(), resultText))->Inc();
        Ctx().AddClientFeaturesBlock();

        return result;
    }
    catch (...) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Exception: " << CurrentExceptionMessage());
    }
}

TResultValue TRequestHandler::RunFormHandler(NSc::TValue* out) {
    if (TResultValue error = RunFormHandler()) {
        return error;
    }

    try {
        // Handlers store result in context so we just need to dump it as a result.
        Ctx().ToJson(out);
        return TResultValue();
    }
    catch (...) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Exception: " << CurrentExceptionMessage());
    }
}

TResultValue TRequestHandler::RunContinuableHandlerImpl(IContinuation::TPtr& resultContinuation) {
    const TStringBuf formName{Ctx().FormName()};
    const TContinuableHandlerFactory* handlerFactory = Context->GlobalCtx().ContinuableHandler(formName);
    if (!handlerFactory) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "No handler for the preparation '" << formName << "' found");
    }

    THolder<TContinuableHandler> handler = (*handlerFactory)();
    if (!handler)
        return TError(TError::EType::SYSTEM, "Unable to create handler");

    resultContinuation = handler->Prepare(*this);
    return TResultValue();
}

TResultValue TRequestHandler::RunContinuableHandler(NSc::TValue* out) {
    try {
        IContinuation::TPtr continuation;
        if (const auto err = RunContinuableHandlerImpl(continuation)) {
            return err;
        }
        Ctx().AddClientFeaturesBlock();
        *out = continuation->ToJson();
        return continuation->IsFinished() ? continuation->GetResult() : TResultValue();
    } catch (...) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Exception: " << CurrentExceptionMessage());
    }
}

TResultValue TRequestHandler::RunContinuableHandler(NAlice::NScenarios::TScenarioRunResponse& out,
                                                    NSc::TValue& outValue) {
    try {
        IContinuation::TPtr continuation;
        if (const auto err = RunContinuableHandlerImpl(continuation)) {
            return err;
        }
        Ctx().AddClientFeaturesBlock();
        outValue = continuation->ToJson();
        out = continuation->AsProtocolRunResponse();
        return continuation->IsFinished() ? continuation->GetResult() : TResultValue();
    } catch (...) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Exception: " << CurrentExceptionMessage());
    }
}

// run handler for the action specified in Context
// output result form to <out>
TResultValue TRequestHandler::RunActionHandler(NSc::TValue* out) {
    try {
        const TString& actionName = Ctx().InputAction()->Name;
        const THandlerFactory* handlerFactory = Context->GlobalCtx().ActionHandler(actionName);
        if (!handlerFactory) {
            return TError(TError::EType::SYSTEM,
                          TStringBuilder() << "No handler for the action '" << actionName << "' found");
        }

        THolder<IHandler> handler = (*handlerFactory)();
        if (!handler) {
            return TError(TError::EType::SYSTEM, TStringBuilder() << "Unable to create action handler for " << actionName);
        }

        const TInstant startTime = TInstant::Now();

        if (TResultValue err = handler->Do(*this)) {
            return err;
        }

        TString signalName = TStringBuilder() << FORM_SIGNAL_PREFIX << actionName << SIGNAL_SUFFIX;
        auto duration = TInstant::Now() - startTime;
        const auto* signal = Context->GlobalCtx().Counters().BassCounters().UnistatHistograms.FindPtr(signalName);
        if (signal) {
            (*signal)->PushSignal(duration.MilliSeconds());
        }
        const auto* signalSLA = Context->GlobalCtx().Counters().BassCounters().UnistatHistograms.FindPtr(signalName + SIGNAL_SLA_SUFFIX);
        if (signalSLA) {
            (*signalSLA)->PushSignal((duration - Context->GlobalCtx().FormSLA(actionName)).MilliSeconds());
        }

        Ctx().ToJson(out);
        return TResultValue();
    }
    catch (...) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Exception: " << CurrentExceptionMessage());
    }
}

} // namespace NBASS
