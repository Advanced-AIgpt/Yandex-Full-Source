#pragma once

#include <util/datetime/base.h>

#define TRACE_LINE(log, arg) if (log) {*log << arg << Endl;}
#define TRACE_STR(log, arg) if (log) {*log << arg;}

namespace NGranet {

inline TString LogPrefix() {
    return Now().FormatLocalTime("[%T] ");
}

} // namespace NGranet
