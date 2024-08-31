#pragma once

#include <alice/bass/libs/fetcher/event_logger.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NTestingHelpers {

class TMockEventLogger : public NHttpFetcher::IEventLogger {
public:
    MOCK_METHOD(void, Debug, (TStringBuf), (const, override));
    MOCK_METHOD(void, Error, (TStringBuf), (const, override));
};

} // namespace NTestingHelpers
