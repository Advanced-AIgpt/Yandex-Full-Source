#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood {

THttpProxyRequest PrepareEntitySearchRequest(const TStringBuf path,
                                             const NScenarios::TRequestMeta& meta,
                                             TRTLogger& logger,
                                             const TString& name = Default<TString>());

void AddEntitySearchRequestItems(TScenarioHandleContext& ctx, const THttpProxyRequest& request);

NJson::TJsonValue RetireEntitySearchResponseItems(const TScenarioHandleContext& ctx);

} // namespace NAlice::NHollywood
