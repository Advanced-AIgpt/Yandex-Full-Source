#pragma once

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/item_names.h>
#include <alice/megamind/library/apphost_request/protos/combinators.pb.h>
#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/library/apphost_request/protos/scenario_errors.pb.h>

#include <alice/megamind/library/scenarios/config_registry/config_registry.h>

#include <alice/megamind/protos/analytics/megamind_analytics_info.pb.h>
#include <alice/megamind/protos/quality_storage/storage.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <util/generic/string.h>

namespace NAlice::NMegamind {

class IPostClassifyState {
public:
    virtual TErrorOr<TMegamindAnalyticsInfo> GetAnalytics() = 0;
    virtual TErrorOr<TQualityStorage> GetQualityStorage() = 0;
    virtual TMaybe<NMegamindAppHost::TScenarioErrorsProto> GetScenarioErrors() = 0;
    virtual TErrorOr<TString> GetWinnerScenario() = 0;
    virtual TStatus GetPostClassifyStatus() = 0;
    virtual TMaybe<TString> GetWinnerCombinator() = 0;
    virtual TMaybe<NMegamindAppHost::TCombinatorProto::ECombinatorStage> GetWinnerCombinatorStage() = 0;
    virtual TMaybe<NScenarios::TScenarioContinueResponse> GetContinueResponse() = 0;
};

class TPostClassifyState final : public IPostClassifyState {
public:
    explicit TPostClassifyState(TItemProxyAdapter& itemAdapter);

    TErrorOr<TMegamindAnalyticsInfo> GetAnalytics() override;
    TErrorOr<TQualityStorage> GetQualityStorage() override;
    TMaybe<NMegamindAppHost::TScenarioErrorsProto> GetScenarioErrors() override;
    [[nodiscard]] TErrorOr<TString> GetWinnerScenario() override;
    [[nodiscard]] TStatus GetPostClassifyStatus() override;
    TMaybe<TString> GetWinnerCombinator() override;
    TMaybe<NMegamindAppHost::TCombinatorProto::ECombinatorStage> GetWinnerCombinatorStage() override;
    TMaybe<NScenarios::TScenarioContinueResponse> GetContinueResponse() override;

private:
    TItemProxyAdapter& ItemAdapter;
};

} // namespace NAlice::NMegamind
