#pragma once

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood {

THttpProxyRequest PrepareDataSyncRequest(const TStringBuf path,
                                         const NScenarios::TRequestMeta& meta,
                                         const TString& uid,
                                         TRTLogger& logger,
                                         const TString& name = Default<TString>(),
                                         const TMaybe<TString> body = Nothing(),
                                         const TMaybe<NAppHostHttp::THttpRequest_EMethod> maybeMethod = Nothing());

void AddDataSyncRequestItems(TScenarioHandleContext& ctx, const THttpProxyRequest& request);

NJson::TJsonValue RetireDataSyncResponseItems(const TScenarioHandleContext& ctx);

TMaybe<NJson::TJsonValue> RetireDataSyncResponseItemsSafe(const TScenarioHandleContext& ctx);

} // namespace NAlice::NHollywood
