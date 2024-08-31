#pragma once

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/config/scenario_protos/config.pb.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/scenarios/interface/scenario.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/scenarios/request.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/analytics/interfaces/analytics_info_builder.h>

namespace NAlice {

class TProtocolScenario : public TScenario {
public:
    using TScenario::TScenario;

    bool IsProtocol() const override {
        return true;
    }

    // FIXME(the0): Should not be a part of the public class interface
    virtual const TScenarioConfig& GetConfig() const = 0;
    virtual TStatus StartRun(const IContext& ctx,
                             const NScenarios::TScenarioRunRequest& request,
                             NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;

    virtual TErrorOr<NScenarios::TScenarioRunResponse> FinishRun(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;

    virtual TErrorOr<NScenarios::TScenarioContinueResponse>
    FinishContinue(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;

    virtual TStatus StartCommit(const IContext& ctx, 
                                const NScenarios::TScenarioApplyRequest& request,
                                NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;

    virtual TErrorOr<NScenarios::TScenarioCommitResponse>
    FinishCommit(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;

    virtual TStatus StartApply(const IContext& ctx,
                               const NScenarios::TScenarioApplyRequest& request,
                               NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;

    virtual TErrorOr<NScenarios::TScenarioApplyResponse>
    FinishApply(const IContext& ctx, NMegamind::TItemProxyAdapter& itemProxyAdapter) const = 0;
};

} // namespace NAlice
