#pragma once

#include "base_suggest_handle.h"

#include <alice/hollywood/library/context/context.h>
#include <alice/hollywood/library/frame/callback.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/suggesters/nlg/register.h>

#include <util/generic/maybe.h>

namespace NAlice::NHollywood {

void Register(NAlice::NNlg::TEnvironment& env) {
    NAlice::NHollywood::NLibrary::NScenarios::NSuggesters::NNlg::RegisterAll(env);
}

template <typename TRecommender, typename TState>
class TStateUpdater {
public:
    TStateUpdater(const TRecommender& recommender, const TBaseSuggestResponseBuilder::TConfig& config,
                  const TMaybe<TSemanticFrame>& frame = Nothing())
        : Nlg(Rng, nullptr, &Register)
        , Recommender(recommender)
        , Config(config)
        , Frame(frame)
    {
    }

    std::unique_ptr<NScenarios::TScenarioRunResponse> ProcessRequest(
        const TScenarioRunRequestWrapper& request);

    const TState& GetState() const {
        return State;
    }

    const TMaybe<TSemanticFrame>& GetFrame() const {
        return Frame;
    }

private:
    NAlice::TFakeRng Rng;
    TCompiledNlgComponent Nlg;
    TRecommender Recommender;
    TBaseSuggestResponseBuilder::TConfig Config;

    TState State;
    TMaybe<TSemanticFrame> Frame;
};

template <typename TRecommender, typename TState>
std::unique_ptr<NScenarios::TScenarioRunResponse> TStateUpdater<TRecommender, TState>::ProcessRequest(
    const TScenarioRunRequestWrapper& request)
{
    auto nlgWrapper = TNlgWrapper::Create(Nlg, request, Rng, ELanguage::LANG_RUS);
    auto response = BuildResponse(request, Recommender, Config, Rng, TRTLogger::NullLogger(), nlgWrapper);

    response->GetResponseBody().GetState().UnpackTo(&State);

    const auto& actions = response->GetResponseBody().GetFrameActions();
    const auto actionIter = actions.find("decline_by_frame");
    if (actionIter != actions.end()) {
        const TMaybe<TFrame> frame = actionIter->second.HasCallback()
            ? GetCallbackFrame(&actionIter->second.GetCallback())
            : Nothing();
        if (frame.Defined()) {
            Frame = frame->ToProto();
        } else {
            Frame = actionIter->second.GetFrame();
        }
    } else {
        Frame = Nothing();
    }

    return response;
}

} // namespace NAlice::NHollywood
