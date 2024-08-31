#include "construct.h"

#include <alice/megamind/library/registry/registry.h>

#include <alice/megamind/library/handlers/apphost_megamind/skr.h>
#include <alice/megamind/library/handlers/apphost_utility/handler.h>

namespace NAlice {

void PopulateRegistry(IGlobalCtx& globalCtx,
                      TMaybe<NInfra::TLogger>& udpLogger,
                      TMaybe<NUdpClickMetrics::TSelfBalancingClient>& udpClient,
                      TRegistry& registry)
{
    NMegamind::RegisterAppHostMegamindHandlers(globalCtx, registry);
    NMegamind::RegisterAppHostUtilityHandlers(globalCtx, registry);

    NMegamind::RegisterAppHostHttpUtilityHandlers(globalCtx, udpLogger, udpClient, registry);
}

} // namespace NAlice
