#pragma once

#include <alice/megamind/library/analytics/megamind_analytics_info.h>
#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/response/response.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/modifiers/modifier_response.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/util/status.h>

namespace NAlice::NMegamind::NModifiers {

class IModifierRequestFactory {
public:
    virtual void SetupModifierRequest(const TRequest& request,
                                      const NScenarios::TScenarioResponseBody& responseBody,
                                      const TString& scenarioName) = 0;
    virtual TStatus ApplyModifierResponse(TScenarioResponse& scenarioResponse, TMegamindAnalyticsInfoBuilder& analyticsInfo) = 0;
};

class TAppHostModifierRequestFactory final : public IModifierRequestFactory {
public:
    TAppHostModifierRequestFactory(TItemProxyAdapter& itemAdapter, const IContext& ctx, const TScenarioConfigRegistry& scenarioConfigRegistry);

    void SetupModifierRequest(const TRequest& request, const NScenarios::TScenarioResponseBody& responseBody,
                              const TString& scenarioName) override;
    TStatus ApplyModifierResponse(TScenarioResponse& scenarioResponse, TMegamindAnalyticsInfoBuilder& analyticsInfo) override;
private:
    TItemProxyAdapter& ItemAdapter;
    const IContext& Ctx;
    const TScenarioConfigRegistry& ScenarioConfigRegistry;
};

} // namespace NAlice::NMegamind::NModifiers
