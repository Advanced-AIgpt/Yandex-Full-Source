#include "parser.h"

#include <library/cpp/testing/unittest/registar.h>

namespace NRTLog {
    Y_UNIT_TEST_SUITE(TestParser) {
        Y_UNIT_TEST(SimpleTest) {
            TYandexioLogParserCounters counters;
            auto parser = MakeYandexioLogParser(counters);
            parser->Parse(TStringBuf{""});
        }
    }
} // namespace NRTLog
