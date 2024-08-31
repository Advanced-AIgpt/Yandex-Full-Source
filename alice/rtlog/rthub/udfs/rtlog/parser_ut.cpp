#include "parser.h"

#include <alice/rtlog/rthub/udfs/rtlog/parser/parser.h>
#include <library/cpp/testing/unittest/registar.h>

namespace NRTLog {
    Y_UNIT_TEST_SUITE(TestParser) {
        Y_UNIT_TEST(SimpleTest) {
            TEventsParserCounters counters;
            auto parser = MakeEventsParser(counters);
            parser->Parse(TStringBuf{""}, false);
        }
    }
} // namespace NRTLog
