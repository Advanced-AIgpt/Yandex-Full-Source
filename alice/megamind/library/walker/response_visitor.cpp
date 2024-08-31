#include "response_visitor.h"

#include "event_count.h"

#include <alice/library/version/version.h>

#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/response/utils.h>
#include <alice/megamind/library/util/slot.h>
#include <alice/megamind/library/util/ttl.h>

#include <alice/megamind/protos/common/gc_memory_state.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/json/json.h>

namespace NAlice {

namespace {

TMaybe<NAlice::TGcMemoryState> ExtractGcMemoryState(const TWizardResponse& wizardResponse) {
    if (wizardResponse.GetProtoResponse().GetAliceGcMemoryStateUpdater().HasMemoryState()) {
        return wizardResponse.GetProtoResponse().GetAliceGcMemoryStateUpdater().GetMemoryState();
    }
    return Nothing();
}

} // namespace

void PrepareSessionBuilder(ISessionBuilder& sessionBuilder, const TString& scenarioName,
                           const TString& productScenarioName,
                           const NMegamind::TStackEngineCore& stackEngineCore,
                           const TDialogHistory& dialogHistory,
                           const NAlice::NScenarios::TLayout& layout,
                           const TMaybe<TSemanticFrame>& responseFrame,
                           const TClientEntities& entities,
                           const TFrameActionsMap& frameActions,
                           const bool requestIsExpected,
                           NMegamind::TModifiersStorage&& modifierStorage,
                           const bool clearModifiersStorage,
                           NAlice::ILightWalkerRequestCtx& walkerCtx,
                           const ui64 lastWhisperTimeMs) {
    sessionBuilder.SetPreviousScenarioName(scenarioName)
        .SetPreviousProductScenarioName(productScenarioName)
        .SetScenarioResponseBuilder(Nothing())
        .SetStackEngineCore(stackEngineCore)
        .SetDialogHistory(dialogHistory)
        .SetLayout(layout)
        .SetResponseFrame(scenarioName != MM_PROTO_VINS_SCENARIO ? responseFrame : Nothing())
        .SetResponseEntities(entities)
        .SetActions(frameActions)
        .SetProtocolInfo(Nothing())
        .SetMegamindAnalyticsInfo(Nothing())
        .SetQualityStorage(Nothing())
        .SetIntentName(Default<TString>())
        .SetRequestIsExpected(requestIsExpected)
        .SetModifiersStorage(clearModifiersStorage ? TMaybe<NMegamind::TModifiersStorage>{} : std::move(modifierStorage))
        .SetProactivityRecommendations({})
        .SetGcMemoryState(ExtractGcMemoryState(walkerCtx.Ctx().Responses().WizardResponse()))
        .SetLastWhisperTimeMs(lastWhisperTimeMs);

    const auto& ctx = walkerCtx.Ctx();
    if (const auto* session = ctx.Session(); session && !ctx.HasExpFlag(EXP_DISABLE_MULTIPLE_SESSIONS)) {
        for (const auto& [name, scenarioSession]: session->GetScenarioSessions()) {
            if (name == scenarioName) {
                continue;
            }
            const auto& scenarioConfig = ctx.ScenarioConfig(name);
            const auto isScenarioEnabled =
                walkerCtx.GlobalCtx().ScenarioConfigRegistry().GetScenarioConfig(name).GetEnabled();
            const auto sessionTimeoutSeconds =
                isScenarioEnabled ? scenarioConfig.GetScenarioSessionTimeoutSeconds()
                                  : walkerCtx.GlobalCtx().Config().GetDisabledScenarioSessionTimeoutSeconds();
            if (NMegamind::IsTimeoutExceeded(
                    scenarioSession.GetTimestamp() / 1000, sessionTimeoutSeconds * 1000,
                    ctx.SpeechKitRequest()->GetRequest().GetAdditionalOptions().GetServerTimeMs()))
            {
                LOG_INFO(ctx.Logger()) << "Scenario session timeout exceeded for scenario '" << name << "'";
                continue;
            }
            auto newScenarioSession = NewScenarioSession(scenarioSession.GetState());
            newScenarioSession.SetTimestamp(scenarioSession.GetTimestamp());
            sessionBuilder.SetScenarioSession(name, newScenarioSession);
        }
    }
}

void UpdateScenarioSession(TSessionProto::TScenarioSession& scenarioSession,
                           const TMaybe<TString> previousScenarioName,
                           const TString& scenarioName,
                           const NScenarios::TScenarioRunResponse::TFeatures& features,
                           const bool filledUntypedRequestedSlot,
                           const bool shouldBecomeActiveScenario,
                           ISessionBuilder& sessionBuilder,
                           const NAlice::IContext& ctx) {
    scenarioSession.SetTimestamp(TInstant::Now().MicroSeconds());
    scenarioSession.SetConsequentIrrelevantResponseCount(UpdateEventCount(
        previousScenarioName,
        ctx.PreviousScenarioSession().GetConsequentIrrelevantResponseCount(),
        scenarioName,
        features.GetIsIrrelevant()
    ));
    scenarioSession.SetConsequentUntypedSlotRequests(UpdateEventCount(
        previousScenarioName,
        ctx.PreviousScenarioSession().GetConsequentUntypedSlotRequests(),
        scenarioName,
        filledUntypedRequestedSlot
    ));

    i32 activityTurn = ctx.PreviousScenarioSession().GetActivityTurn();
    if (features.GetIgnoresExpectedRequest()) {
        activityTurn = 0;
    }
    scenarioSession.SetActivityTurn(UpdateEventCount(
        previousScenarioName,
        activityTurn,
        scenarioName,
        shouldBecomeActiveScenario
    ));

    sessionBuilder.SetScenarioSession(scenarioName, scenarioSession);
}

// TFinalizerVisitor -----------------------------------------------------------
TResponseFinalizer::TResponseFinalizer(TScenarioWrapperPtr wrapper, ILightWalkerRequestCtx& walkerCtx,
                                       const TRequest& request, const TString& scenarioName,
                                       const TFeatures& features, bool requestIsExpected)
    : Wrapper{wrapper}
    , WalkerCtx{walkerCtx}
    , Request{request}
    , ScenarioName{scenarioName}
    , Features{features}
    , RewrittenRequest{WalkerCtx.Ctx().Responses().BegemotResponseRewrittenRequestResponse().GetData()}
    , RequestIsExpected{requestIsExpected}
{
}

TStatus TResponseFinalizer::Finalize(TResponseBuilder* builder) {
    if (!builder) {
        return TError{TError::EType::Logic} << "Unable to finalize response: no builder";
    }
    builder->SetVersion(NAlice::VERSION_STRING);
    const auto& ctx = WalkerCtx.Ctx();

    THolder<ISessionBuilder> sessionBuilder;
    if (ctx.Session() && ctx.ScenarioConfig(ScenarioName).GetPureSession()) {
        sessionBuilder = ctx.Session()->GetUpdater();
    } else {
        sessionBuilder = ctx.Session() ? ctx.Session()->CreateBuilder() : MakeSessionBuilder();

        const auto response = builder->GetRenderedResponse();
        const bool shouldLeaveOriginalStackEngine =
            Wrapper->IsApplyNeededOnWarmUpRequestWithSemanticFrame() || !builder->GetStackEngine();
        const auto& stackEngineCore =
            shouldLeaveOriginalStackEngine ? ctx.StackEngineCore() : builder->GetStackEngine()->GetCore();
        const auto& layout =
            builder->GetLayout() ? *builder->GetLayout() : NScenarios::TLayout::default_instance();
        UpdateSession(response, *sessionBuilder, builder->GetSemanticFrame(), builder->GetEntities(),
                      builder->GetActions(), stackEngineCore, layout, builder->GetProductScenarioName());
    }

    const auto dialogId = Request.GetDialogId().GetOrElse("");
    builder->SetSession(dialogId, sessionBuilder->Build()->Serialize());
    return Success();
}

void TResponseFinalizer::UpdateSession(const TString& response,
                                       ISessionBuilder& sessionBuilder,
                                       const TMaybe<TSemanticFrame>& responseFrame,
                                       const TClientEntities& entities,
                                       const TFrameActionsMap& frameActions,
                                       const NMegamind::TStackEngineCore& stackEngineCore,
                                       const NAlice::NScenarios::TLayout& layout,
                                       const TString& productScenarioName) {
    const auto& ctx = WalkerCtx.Ctx();
    const TDeque<TString> currentTurnDialogHistory = {ctx.PolyglotUtterance(), response};

    TDialogHistory dialogHistory;
    bool filledUntypedRequestedSlot = false;
    if (const ISession* session = ctx.Session()) {
        dialogHistory = session->GetDialogHistory();
        if (session->GetPreviousScenarioName() == ScenarioName) {
            const auto filledRequestedSlot = GetFilledRequestedSlot(Wrapper->GetSemanticFrames(), session->GetResponseFrame());
            if (filledRequestedSlot) {
                filledUntypedRequestedSlot = (filledRequestedSlot->GetType() == "string");
            }
        }
    }

    const TMaybe<TString> previousScenarioName = ctx.Session() ? ctx.Session()->GetPreviousScenarioName() : TMaybe<TString>{};

    const bool clearModifiersStorage =
        ctx.HasExpFlag(EXP_PROACTIVITY_RESET_SESSION_LIKE_VINS) &&
        previousScenarioName != ScenarioName;

    dialogHistory.PushDialogTurn({ctx.PolyglotUtterance(), RewrittenRequest, response, ScenarioName, Request.GetServerTimeMs(),
                                  ctx.ClientInfo().Epoch * 1000});

    PrepareSessionBuilder(sessionBuilder, ScenarioName, productScenarioName, stackEngineCore, dialogHistory, layout,
                          responseFrame, entities, frameActions, RequestIsExpected,
                          std::move(Wrapper->GetModifiersStorage()), clearModifiersStorage, WalkerCtx,
                          Request.GetWhisperInfo().Defined() ? Request.GetWhisperInfo()->GetUpdatedLastWhisperTimeMs() : 0);

    auto scenarioSession = NewScenarioSession(Wrapper->GetApplyEnv(Request, ctx).State);
    UpdateScenarioSession(scenarioSession, previousScenarioName, ScenarioName, Features.GetScenarioFeatures(),
                          filledUntypedRequestedSlot, Wrapper->ShouldBecomeActiveScenario(), sessionBuilder, ctx);
}

} // namespace NAlice
