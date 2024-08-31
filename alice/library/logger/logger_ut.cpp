#include "logger_utils.h"

#include <library/cpp/logger/stream.h>
#include <library/cpp/logger/filter.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/serialized_enum.h>

namespace {

using namespace NAlice::NLogging;

Y_UNIT_TEST_SUITE(Logger) {
    Y_UNIT_TEST(LogLineLevels) {
        const auto levels = GetEnumNames<ELogPriority>();
        for (const auto& level : levels) {
            bool isOver = false;
            for (const auto& testLevel : levels) {
                TStringStream ss;
                TLog log{MakeHolder<TFilteredLogBackend>(MakeHolder<TStreamLogBackend>(&ss), level.first)};

                LogLine(log, testLevel.first, " <reqid>", { "file", 1 }, "line");

                if (!isOver) {
                    UNIT_ASSERT_C(!ss.Empty(),
                                  TStringBuilder{}
                                      << "log must not be empty: logger level ("
                                      << level.second << "), line level: " << testLevel.second);
                    isOver = level.first == testLevel.first;
                } else {
                    UNIT_ASSERT_C(ss.Empty(),
                                  TStringBuilder{}
                                      << "log must be empty: logger level ("
                                      << level.second << "), line level: " << testLevel.second);
                }
            }
        }
    }
}

} // namespace
