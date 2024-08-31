#pragma once

#include <alice/megamind/library/context/context.h>
#include <alice/megamind/library/new_modifiers/modifier_request_factory.h>
#include <alice/megamind/library/response/response.h>

#include <alice/megamind/protos/scenarios/response.pb.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind::NTesting {

class TMockModifierRequestFactory : public NModifiers::IModifierRequestFactory {
public:

    TMockModifierRequestFactory() {
        ON_CALL(*this, ApplyModifierResponse).WillByDefault(testing::Return(Success()));
    }

    MOCK_METHOD(void, SetupModifierRequest,
                (const TRequest& request, const NScenarios::TScenarioResponseBody& responseBody,
                 const TString& scenarioName),
                (override));

    MOCK_METHOD(TStatus, ApplyModifierResponse, (TScenarioResponse& scenarioResponse, TMegamindAnalyticsInfoBuilder& analyticsInfo), (override));
};

} // namespace NAlice::NMegamind::NTesting
