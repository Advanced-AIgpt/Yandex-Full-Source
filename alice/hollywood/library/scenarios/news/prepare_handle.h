#pragma once

#include "alice/hollywood/library/framework/core/request.h"
#include "news_fast_data.h"

#include <alice/hollywood/library/scenarios/news/proto/apply_arguments.pb.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/framework/framework.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/logger/fwd.h>
#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/memento/proto/user_configs.pb.h>

using namespace ru::yandex::alice::memento::proto;

namespace NAlice::NHollywood {

namespace NImpl {

std::variant<THttpProxyRequest, NScenarios::TScenarioRunResponse> NewsPrepareDoImpl(
    TContext& ctx,
    TNlgWrapper& nlg,
    TRunResponseBuilder& builder,
    const TScenarioRunRequestWrapper& request,
    const NScenarios::TRequestMeta& meta,
    const NJson::TJsonValue& appHostParams,
    NAlice::IRng& rng,
    const NHollywoodFw::TRunRequest& runRequest);

} // namespace NImpl

class TBassNewsPrepareHandle : public TScenario::THandleBase {
    TString Name() const override {
        return "prepare";
    }

    void Do(TScenarioHandleContext& ctx) const override;
};

} // namespace NAlice::NHollywood
