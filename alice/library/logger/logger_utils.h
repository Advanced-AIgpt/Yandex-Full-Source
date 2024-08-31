#pragma once

#include <library/cpp/logger/log.h>

#include <util/system/src_location.h>

namespace NAlice::NLogging {

void LogLine(TLog& log, ELogPriority priority, TStringBuf reqId, const TSourceLocation& loc, TStringBuf data);

} // namespace NAlice::NLogging
