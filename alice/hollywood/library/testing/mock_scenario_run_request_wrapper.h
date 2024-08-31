#pragma once

#include <alice/hollywood/library/request/fwd.h>
#include <alice/hollywood/library/request/request.h>

#include <alice/library/logger/logger.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NHollywood {

class TMockScenarioRunRequestWrapper : public TScenarioRunRequestWrapper {
public:
    TMockScenarioRunRequestWrapper()
        : TScenarioRunRequestWrapper{NScenarios::TScenarioRunRequest{}, NAppHost::NService::TTestContext{}} {
        ON_CALL(*this, Proto())
            .WillByDefault(testing::ReturnRef(TScenarioRunRequestWrapper::TProto::default_instance()));
        ON_CALL(*this, BaseRequestProto())
            .WillByDefault(testing::ReturnRef(NScenarios::TScenarioBaseRequest::default_instance()));
        ON_CALL(*this, HasExpFlag).WillByDefault(testing::Return(false));
    }

    MOCK_METHOD(const TScenarioRunRequestWrapper::TProto&, Proto, (), (const));
    MOCK_METHOD(const NScenarios::TScenarioBaseRequest&, BaseRequestProto, (), (const));
    MOCK_METHOD(bool, HasExpFlag, (TStringBuf name), (const));
};

} // namespace NAlice::NHollywood
