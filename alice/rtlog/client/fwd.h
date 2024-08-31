#pragma once

#include <memory>

namespace NRTLog {
    class TClient;
    class TRequestLogger;
    using TRequestLoggerPtr = std::shared_ptr<TRequestLogger>;
} // namespace NRTLog
