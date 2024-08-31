#pragma once

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/global_context/global_context.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NHollywood {

void DispatchScenarioHandle(const TScenario& scenario,
                            const TScenario::THandleBase& handle,
                            TGlobalContext& globalContext,
                            NAppHost::IServiceContext& ctx,
                            const TScenarioNewContext* runRequest = nullptr);

} // namespace NAlice::NHollywood
