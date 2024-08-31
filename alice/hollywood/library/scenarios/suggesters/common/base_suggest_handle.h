#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/suggesters/common/suggest_response_builder.h>

#include <memory>

namespace NAlice::NHollywood {

template <typename TRecommender>
std::unique_ptr<NScenarios::TScenarioRunResponse> BuildResponse(
    const TScenarioRunRequestWrapper& request,
    const TRecommender& recommender,
    const TBaseSuggestResponseBuilder::TConfig& config,
    IRng& rng,
    TRTLogger& logger,
    TNlgWrapper& nlgWrapper);

template <typename TRecommender>
class TBaseSuggestHandle : public TScenario::THandleBase {
public:
    TString Name() const override {
        return "run";
    }

    void Do(TScenarioHandleContext& ctx) const override {
        const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

        const auto& recommender = ctx.Ctx.ScenarioResources<TRecommender>();

        TNlgWrapper nlgWrapper = TNlgWrapper::Create(ctx.Ctx.Nlg(), request, ctx.Rng, ctx.UserLang);
        const auto response = BuildResponse(request, recommender, BuildConfig(request),
                                            ctx.Rng, ctx.Ctx.Logger(), nlgWrapper);
        ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
    }

protected:
    virtual TBaseSuggestResponseBuilder::TConfig BuildConfig(const TScenarioRunRequestWrapper& request) const = 0;
};

} // namespace NAlice::NHollywood
