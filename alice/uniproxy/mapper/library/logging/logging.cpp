#include "logging.h"

#include <library/cpp/logger/log.h>

#include <util/datetime/base.h>
#include <util/generic/scope.h>
#include <util/string/builder.h>
#include <util/string/printf.h>

namespace NAlice::NUniproxy {
    namespace {
        TString FormatWithTimestamp(ELogPriority, TStringBuf data) {
            return TStringBuilder() << "[" << TInstant::Now().FormatLocalTime("%Y-%m-%d %H:%M:%S") << "] " << data;
        }
    }

    void SetTimestampFormatter(TLog& logger) noexcept {
        logger.SetFormatter(&FormatWithTimestamp);
    }

    void Log(TLog& logger, ELogPriority priority, const char* format, ...) {
        TString result;
        va_list args;
        va_start(args, format);

        Y_DEFER {
            va_end(args);
        };
        vsprintf(result, format, args);
        logger.Write(priority, result);
    }
}
