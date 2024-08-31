#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NRTLog {
    class TEventsParser;
    struct TEventsParserCounters;

    THolder<TEventsParser> MakeEventsParser(const TEventsParserCounters& counters);
}
