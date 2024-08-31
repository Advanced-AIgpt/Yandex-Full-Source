#include "parser.h"

#include <alice/rtlog/rthub/udfs/rtlog/parser/parser.h>

namespace NRTLog {
    THolder<TEventsParser> MakeEventsParser(const TEventsParserCounters& counters) {
        return MakeHolder<TEventsParser>(counters);
    }
}
