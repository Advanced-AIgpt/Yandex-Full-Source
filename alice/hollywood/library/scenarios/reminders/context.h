#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>

namespace NAlice::NHollywood::NReminders {

class THandlerContext {
public:
    THandlerContext(TScenarioHandleContext& ctx)
        : Ctx_{ctx}
        , Proto_{GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM)}
        , Request_{Proto_, Ctx_.ServiceCtx}
    {
    }

    TScenarioHandleContext* operator->() {
        return &Ctx_;
    }

    const TScenarioRunRequestWrapper& Request() {
        return Request_;
    }

private:
    TScenarioHandleContext& Ctx_;

    const NScenarios::TScenarioRunRequest Proto_;
    const TScenarioRunRequestWrapper Request_;
};

} // namespace NAlice::NHollywood::NReminders
