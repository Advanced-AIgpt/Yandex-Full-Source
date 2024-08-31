#include "logger_utils.h"

#include <util/datetime/base.h>
#include <util/string/cast.h>

namespace NAlice::NLogging {

void LogLine(TLog& log, ELogPriority priority, TStringBuf reqId, const TSourceLocation& loc, TStringBuf data) {
    log << priority
        << ToString(priority) << TStringBuf(": ") << TInstant::Now().ToString()
        << reqId << ' '
        << loc.File << ':' << loc.Line << ' '
        << data << Endl;
}

} // namespace NAlice::NLogging
