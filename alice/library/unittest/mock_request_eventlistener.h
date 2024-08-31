#pragma once

#include <alice/bass/libs/fetcher/neh.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NTestingHelpers {

class TMockRequestEventListener : public NHttpFetcher::IRequestEventListener {
public:
    MOCK_METHOD(void, OnAttempt, (NHttpFetcher::THandle::TRef), (override));
    MOCK_METHOD(void, AddHandleObserver, (NHttpFetcher::THandle::TRef, NHttpFetcher::IRequestEventListener::TCallback), (override));
};

} // namespace NAlice::NTestingHelpers
