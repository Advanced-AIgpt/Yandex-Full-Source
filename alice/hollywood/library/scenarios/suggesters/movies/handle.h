#pragma once

#include "recommender.h"

#include <alice/hollywood/library/scenarios/suggesters/common/base_suggest_handle.h>

#include <alice/hollywood/library/scenarios/suggesters/nlg/register.h>

namespace NAlice::NHollywood {

TBaseSuggestResponseBuilder::TConfig BuildMovieSuggestConfig();

template <>
std::unique_ptr<NScenarios::TScenarioRunResponse> BuildResponse<TMovieRecommender>(
    const TScenarioRunRequestWrapper& request,
    const TMovieRecommender& recommender,
    const TBaseSuggestResponseBuilder::TConfig& config,
    IRng& rng, TRTLogger& logger,
    TNlgWrapper& nlgWrapper);

class TMovieSuggestRunHandle : public TBaseSuggestHandle<TMovieRecommender> {
public:
    TMovieSuggestRunHandle();

private:
    TBaseSuggestResponseBuilder::TConfig Config;

private:
    TBaseSuggestResponseBuilder::TConfig BuildConfig(const TScenarioRunRequestWrapper& request) const override;
};

}  // namespace NAlice::NHollywood
