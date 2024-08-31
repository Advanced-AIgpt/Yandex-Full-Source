#pragma once

#include <library/cpp/logger/log.h>

namespace NAlice::NUniproxy {
    void SetTimestampFormatter(TLog& logger) noexcept;
    void Log(TLog& logger, ELogPriority priority, char const* format, ...);
}
