#pragma once

#include <infra/udp_click_metrics/client/client.h>

#include <library/cpp/neh/rpc.h>

namespace NAlice {

class IGlobalCtx;
class TRegistry;

namespace NMegamind {

namespace NImpl {

void UtilityHandlerHttp(const TString& path,
                        IGlobalCtx& globalCtx,
                        TMaybe<NInfra::TLogger>& udpLogger,
                        TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient,
                        const NNeh::IRequestRef& req);

} // namespace NImpl

void RegisterAppHostUtilityHandlers(IGlobalCtx& globalCtx, TRegistry& registry);
void RegisterAppHostHttpUtilityHandlers(IGlobalCtx& globalCtx,
                                        TMaybe<NInfra::TLogger>& udpLogger,
                                        TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient,
                                        TRegistry& registry);
}

} // namespace NAlice::NAlice
