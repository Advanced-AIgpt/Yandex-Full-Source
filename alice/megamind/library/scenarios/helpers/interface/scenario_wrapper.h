#pragma once

#include "scenario_ref.h"

#include <alice/megamind/library/scenarios/interface/data_sources.h>
#include <alice/megamind/library/scenarios/interface/scenario_env.h>

#include <alice/megamind/library/apphost_request/protos/scenario.pb.h>
#include <alice/megamind/library/request/event/event.h>
#include <alice/megamind/library/session/protos/state.pb.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/modifiers/modifiers.pb.h>

#include <kernel/factor_storage/factor_storage.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAlice {

class IContext;

class TScenarioResponse;
class TQualityStorage;

enum class EApplicability {
    Inapplicable = 0,
    Applicable = 1
};

enum class EDeferredApplyMode {
    DeferredCall,
    DeferApply,
    DontDeferApply,
    WarmUp,
};

enum EApplyResult { Called, Deferred };

class IScenarioWrapper : public IScenarioRef {
public:
    using TSemanticFrames = TVector<TSemanticFrame>;

public:
    virtual TLightScenarioEnv GetApplyEnv(const TRequest& request, const IContext& ctx) = 0;
    virtual TScenarioEnv GetEnv(const TRequest& request, const IContext& ctx) = 0;
    virtual const TSemanticFrames& GetSemanticFrames() const = 0;
    virtual NMegamind::TAnalyticsInfoBuilder& GetAnalyticsInfo() = 0;
    virtual const NMegamind::TAnalyticsInfoBuilder& GetAnalyticsInfo() const = 0;
    virtual NMegamind::TUserInfoBuilder& GetUserInfo() = 0;
    virtual const NMegamind::TUserInfoBuilder& GetUserInfo() const = 0;
    virtual NMegamind::TMegamindAnalyticsInfo& GetMegamindAnalyticsInfo() = 0;
    virtual const TQualityStorage& GetQualityStorage() const = 0;
    virtual TQualityStorage& GetQualityStorage() = 0;
    virtual const NMegamind::TModifiersStorage& GetModifiersStorage() const = 0;
    virtual NMegamind::TModifiersStorage& GetModifiersStorage() = 0;
    virtual TStatus Init(const TRequest& request, const IContext& ctx, NMegamind::IDataSources& dataSources) = 0;
    virtual std::once_flag& GetAskFlag() = 0;
    virtual TStatus Ask(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;
    virtual TStatus Finalize(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;
    virtual std::once_flag& GetContinueFlag() = 0;
    virtual TStatus StartHeavyContinue(const TRequest& request, const IContext& ctx) = 0;
    virtual TStatus FinishContinue(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;
    virtual EApplicability SetReasonWhenNonApplicable(const TRequest& request, const IContext& ctx,
                                                      TScenarioResponse& response) = 0;
    virtual EDeferredApplyMode GetDeferredApplyMode() const = 0;
    virtual TErrorOr<EApplyResult> StartApply(const TRequest& request, const IContext& ctx, TScenarioResponse& response,
                                              const NMegamind::TMegamindAnalyticsInfo& megamindAnalyticsInfo,
                                              const TQualityStorage& storage, const TProactivityAnswer& proactivity) = 0;
    virtual TErrorOr<EApplyResult> FinishApply(const TRequest& request, const IContext& ctx, TScenarioResponse& response) = 0;
    virtual bool IsSuccess() const = 0;
    virtual bool ShouldBecomeActiveScenario() const = 0;
    virtual NMegamindAppHost::TScenarioProto GetScenarioProto() const = 0;
    virtual TStatus RestoreInit(NMegamind::TItemProxyAdapter& itemAdapter) = 0;
    virtual bool IsApplyNeededOnWarmUpRequestWithSemanticFrame() const = 0;
};

using TScenarioWrapperPtr = TIntrusivePtr<IScenarioWrapper>;
using TScenarioWrapperPtrs = TVector<TScenarioWrapperPtr>;
using TPostAnalyticsFiller = std::function<void(TScenarioWrapperPtr)>;

}  // namespace NAlice
