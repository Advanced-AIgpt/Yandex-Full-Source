#pragma once

#include "alice/hollywood/library/framework/core/request.h"
#include "news_block.h"
#include "news_fast_data.h"

#include <alice/hollywood/library/scenarios/news/proto/news.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/megamind/protos/scenarios/request_meta.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood {

namespace NImpl {
    const unsigned int TEASER_NEWS_COUNT = 5u;
    const unsigned int TEASER_NEWS_PREVIEW_COUNT = 1u;

    std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse> NewsRenderInCallbackMode(
        const TScenarioRunRequestWrapper& runRequest,
        const NHollywoodFw::TRunRequest& runRequestNew,
        const NJson::TJsonValue& bassResponse,
        NHollywood::TContext& ctx,
        TRunResponseBuilder& builder,
        const TNewsBlock& directive,
        const TNewsFastData& fastData,
        NAlice::IRng& rng);

    std::unique_ptr<NAlice::NScenarios::TScenarioRunResponse> NewsRenderDoImpl(
        const NHollywood::TScenarioRunRequestWrapper& runRequest,
        const NHollywoodFw::TRunRequest& runRequestNew,
        const NJson::TJsonValue& bassResponse,
        NHollywood::TContext& ctx,
        TRunResponseBuilder& builder,
        NAlice::IRng& rng);

    void SetSourceChangePostrollVoiceButton(std::unique_ptr<NScenarios::TScenarioRunResponse>& scenarioResponse);

    void SetNewsBlockVoiceButton(
        std::unique_ptr<NScenarios::TScenarioRunResponse>& scenarioResponse,
        const NJson::TJsonValue& bassResponse,
        TNewsBlockVoiceButton& voiceButton);
} // namespace NImpl

class TBassNewsRenderHandle : public TScenario::THandleBase {
    TString Name() const override {
        return "render";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
