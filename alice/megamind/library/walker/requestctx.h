#pragma once

#include "response.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/classifiers/pre.h>
#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/context/responses.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/new_modifiers/modifier_request_factory.h>
#include <alice/megamind/library/request/request.h>
#include <alice/megamind/library/response/combinator_response.h>
#include <alice/megamind/library/requestctx/requestctx.h>
#include <alice/megamind/library/search/request.h>
#include <alice/megamind/library/speechkit/request_build.h>
#include <alice/megamind/library/stage_wrappers/postclassify_state.h>

#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>

#include <dj/services/alisa_skills/server/proto/client/proactivity_response.pb.h>

#include <kernel/factor_storage/factor_storage.h>

#include <util/generic/maybe.h>
#include <util/generic/ptr.h>

#include <memory>

namespace NAlice {

class TWebSearchRequestBuilder;

class ILightWalkerRequestCtx {
public:
    enum class ERunStage {
        // Run request
        PreClassification = 110 /* "pre_classification" */,
        Prepare = 120 /* "prepare" */,

        // Apply Request
        ApplyPrepareScenario = 210 /* apply_prepare_scenario */,
        ApplyRenderScenario = 220 /* apply_render_scenario */,
        ModifyApplyScenario = 230 /* modify_apply_scenario */,

        // Walker Run division
        PostClassification = 310 /* "post_classification" */,
        ProcessContinue = 320 /* "process_continue" */,
        RunFinalize = 330 /* "run_finalize" */,
        ClassifyWinner = 340 /* "classify_winner" */,
        ProcessCombinatorContinue = 350 /* "process_combinator_continue" */,
    };

public:
    virtual ~ILightWalkerRequestCtx() = default;

    virtual TRequestCtx& RequestCtx() = 0;
    virtual const TRequestCtx& RequestCtx() const = 0;

    virtual IContext& Ctx() = 0;
    virtual const IContext& Ctx() const = 0;

    virtual IRng& Rng() = 0;

    virtual ERunStage RunStage() const = 0;

    virtual NMegamind::TItemProxyAdapter& ItemProxyAdapter() = 0;

    virtual NMegamind::IPostClassifyState& PostClassifyState();

    virtual NMegamind::NModifiers::IModifierRequestFactory& ModifierRequestFactory() = 0;

    // Helpers.
    IGlobalCtx& GlobalCtx() {
        return RequestCtx().GlobalCtx();
    }
    const IGlobalCtx& GlobalCtx() const {
        return RequestCtx().GlobalCtx();
    }
};

class IRunWalkerRequestCtx : virtual public ILightWalkerRequestCtx {
public:
    virtual TFactorStorage& FactorStorage() = 0;

    virtual void MakeSearchRequest(TWebSearchRequestBuilder& builder, const IEvent& event) = 0;

    virtual void MakeProactivityRequest(const TRequest& requestModel,
                                        const TScenarioToRequestFrames& scenarioToFrames,
                                        const NMegamind::TProactivityStorage& storage) = 0;

    virtual void SavePostClassifyState(const TWalkerResponse& walkerResponse,
                                       const NMegamind::TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                       TStatus postClassifyError, const TScenarioWrapperPtr winnerScenario,
                                       const TRequest& request);

    virtual void SaveCombinatorState(const NMegamind::TCombinatorResponse& combinatorResponse,
                                     const TRequest& request);
};


} // namespace NAlice
