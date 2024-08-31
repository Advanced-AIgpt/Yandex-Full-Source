#pragma once

#include <infra/udp_click_metrics/client/client.h>

namespace NAlice {

class IGlobalCtx;
class TRegistry;

void PopulateRegistry(IGlobalCtx& globalCtx,
                      TMaybe<NInfra::TLogger>& udpLogger,
                      TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient,
                      TRegistry& registry);

} // namespace NAlice
