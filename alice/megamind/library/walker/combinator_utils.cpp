#include "combinator_utils.h"

#include "event_count.h"

#include <alice/megamind/library/util/ttl.h>

#include <alice/library/version/version.h>

namespace NAlice::NMegamind {

void UpdateCombinatorSession(ISessionBuilder& sessionBuilder,
                             const TClientEntities& entities,
                             const TFrameActionsMap& frameActions,
                             const NMegamind::TStackEngineCore& stackEngineCore,
                             const NAlice::NScenarios::TLayout& layout,
                             const TString& productScenarioName,
                             IRunWalkerRequestCtx& walkerCtx,
                             const NScenarios::TCombinatorResponse& combinatorResponse,
                             IScenarioWalker::TPreClassifyState& preClassifyState,
                             const TString& combinatorName,
                             const TRequest& request) {
    const auto& ctx = walkerCtx.Ctx();

    // TODO process dialogHistory
    TDialogHistory dialogHistory;

    const TMaybe<TString> previousScenarioName = ctx.Session() ? ctx.Session()->GetPreviousScenarioName() : TMaybe<TString>{};

    PrepareSessionBuilder(sessionBuilder, combinatorName, productScenarioName, stackEngineCore, dialogHistory, layout,
                          Nothing(), // TODO response frame
                          entities, frameActions, /* requestIsExpected= */ false,
                          NMegamind::TModifiersStorage{}, // TODO save modifier storage
                          /* clearModifiersStorage= */ false, walkerCtx,
                          request.GetWhisperInfo().Defined() ? request.GetWhisperInfo()->GetUpdatedLastWhisperTimeMs() : 0);

    const auto& features = combinatorResponse.GetResponse().GetFeatures();
    for (const auto& scenarioName : combinatorResponse.GetUsedScenarios()) {
        const auto* wrapper = FindIfPtr(preClassifyState.ScenarioWrappers, [&scenarioName] (TScenarioWrapperPtr sw) {
            return sw->GetScenario().GetName() == scenarioName;
        });
        if (wrapper) {
            auto scenarioSession = NewScenarioSession((*wrapper)->GetApplyEnv(request, ctx).State);
            UpdateScenarioSession(scenarioSession, previousScenarioName, scenarioName, features,
                                  /* filledUntypedRequestedSlot= */ false,
                                  (*wrapper)->ShouldBecomeActiveScenario(), sessionBuilder, ctx);
        }
    }

    auto newScenarioSession = NewScenarioSession(/* state= */ {});
    newScenarioSession.SetTimestamp(TInstant::Now().MicroSeconds());
    sessionBuilder.SetScenarioSession(combinatorName, newScenarioSession);
}

void FinalizeCombinator(TResponseBuilder& builder, IRunWalkerRequestCtx& walkerCtx, const TRequest& request,
                        const NScenarios::TCombinatorResponse& combinatorResponse,
                        IScenarioWalker::TPreClassifyState& preClassifyState, const TString& combinatorName) {
    builder.SetVersion(NAlice::VERSION_STRING);
    const auto& ctx = walkerCtx.Ctx();

    THolder<ISessionBuilder> sessionBuilder;
    sessionBuilder = ctx.Session() ? ctx.Session()->CreateBuilder() : MakeSessionBuilder();

    const auto response = builder.GetRenderedResponse();
    const auto& stackEngineCore =
        builder.GetStackEngine() ? builder.GetStackEngine()->GetCore() : ctx.StackEngineCore();
    const auto& layout =
        builder.GetLayout() ? *builder.GetLayout() : NScenarios::TLayout::default_instance();

    UpdateCombinatorSession(*sessionBuilder, builder.GetEntities(), builder.GetActions(), stackEngineCore,
                            layout, builder.GetProductScenarioName(), walkerCtx, combinatorResponse,
                            preClassifyState, combinatorName, request);


    const auto dialogId = request.GetDialogId().GetOrElse("");
    builder.SetSession(dialogId, sessionBuilder->Build()->Serialize());
}

} // namespace NAlice::NMegamind
