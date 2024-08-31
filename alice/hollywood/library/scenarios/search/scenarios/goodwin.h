#pragma once

#include "base.h"

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/search/utils/serp_helpers.h>
#include <alice/hollywood/library/frame_filler/lib/frame_filler_handlers.h>
#include <alice/hollywood/library/scenarios/goodwin/handlers/goodwin_handlers.h>

namespace NAlice::NHollywood::NSearch {
    class TEntitySearchGoodwinScenario: public TSearchScenario {
    public:
        explicit TEntitySearchGoodwinScenario(TSearchContext& ctx);

        bool Do(const TSearchResult& response) override;

    private:
        NFrameFiller::TGoodwinScenarioRunHandler RunHandler;
    };

    class TGoodwinApplyScenario {
    public:
        TGoodwinApplyScenario(TContext& ctx);

        TMaybe<NScenarios::TScenarioApplyResponse> Do(const TScenarioApplyRequestWrapper& request);

    private:
        TContext& Ctx;
        NFrameFiller::TGoodwinScenarioApplyHandler RunHandler;
    };

    class TGoodwinCommitScenario {
    public:
        TGoodwinCommitScenario(TContext& ctx);

        TMaybe<NScenarios::TScenarioCommitResponse> Do(const TScenarioApplyRequestWrapper& request);

    private:
        TContext& Ctx;
        NFrameFiller::TGoodwinScenarioCommitHandler RunHandler;
    };

}
