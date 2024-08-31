#include "config.h"

namespace NAlice {

ui16 MonitoringPort(const TConfig& config) {
    const auto& monServiceConfig = config.GetMonService();

    if (monServiceConfig.HasPort()) {
        const ui32 port = monServiceConfig.GetPort();
        Y_ENSURE(port > 0 && port < Max<ui16>());
        return port;
    }

    Y_ENSURE(monServiceConfig.HasPortOffset());
    Y_ENSURE(config.HasAppHost() && config.GetAppHost().HasHttpPort());

    const i64 port = static_cast<i64>(config.GetAppHost().GetHttpPort()) + monServiceConfig.GetPortOffset();
    Y_ENSURE(port > 0 && port < Max<ui16>());
    return static_cast<ui16>(port);
}

} // namespace NAlice
