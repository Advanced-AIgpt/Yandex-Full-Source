#pragma once

#include "requestctx.h"

#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>

#include <util/generic/strbuf.h>

namespace NAlice {

using TFrameActionsMap = ::google::protobuf::Map<TString, NScenarios::TFrameAction>;
using TClientEntities = ::google::protobuf::RepeatedPtrField<TClientEntity>;

class TResponseFinalizer {
public:
    TResponseFinalizer(TScenarioWrapperPtr wrapper, ILightWalkerRequestCtx& walkerCtx,
                       const TRequest& request, const TString& scenarioName,
                       const TFeatures& features, bool RequestIsExpected);

    TStatus Finalize(TResponseBuilder* builder);

private:
    void UpdateSession(const TString& response,
                       ISessionBuilder& sessionBuilder,
                       const TMaybe<TSemanticFrame>& responseFrame,
                       const TClientEntities& entities,
                       const TFrameActionsMap& frameActions,
                       const NMegamind::TStackEngineCore& stackEngineCore,
                       const NScenarios::TLayout& layout,
                       const TString& productScenarioName);

private:
    TScenarioWrapperPtr Wrapper;
    ILightWalkerRequestCtx& WalkerCtx;
    const TRequest& Request;
    const TString& ScenarioName;
    TFeatures Features;
    const TString& RewrittenRequest;
    bool RequestIsExpected;
};

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
                           const ui64 lastWhisperTime);

void UpdateScenarioSession(TSessionProto::TScenarioSession& scenarioSession,
                           const TMaybe<TString> previousScenarioName,
                           const TString& scenarioName,
                           const NScenarios::TScenarioRunResponse::TFeatures& features,
                           const bool filledUntypedRequestedSlot,
                           const bool shouldBecomeActiveScenario,
                           ISessionBuilder& sessionBuilder,
                           const NAlice::IContext& ctx);

} // namespace NAlice
