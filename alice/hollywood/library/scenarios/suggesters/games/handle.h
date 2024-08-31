#pragma once

#include "recommender.h"

#include <alice/hollywood/library/scenarios/suggesters/common/base_suggest_handle.h>

#include <alice/hollywood/library/scenarios/suggesters/nlg/register.h>

#include <memory>

namespace NAlice::NHollywood {

TBaseSuggestResponseBuilder::TConfig BuildGamesSuggestConfig(bool useOnboardingFrame = false);

template <>
std::unique_ptr<NScenarios::TScenarioRunResponse> BuildResponse<TGameRecommender>(
    const TScenarioRunRequestWrapper& request,
    const TGameRecommender& recommender,
    const TBaseSuggestResponseBuilder::TConfig& config,
    IRng& rng, TRTLogger& logger,
    TNlgWrapper& nlgWrapper);

class TGameSuggestRunHandle : public TBaseSuggestHandle<TGameRecommender> {
private:
    TBaseSuggestResponseBuilder::TConfig BuildConfig(const TScenarioRunRequestWrapper& request) const override;
};

} // namespace NAlice::NHollywood
