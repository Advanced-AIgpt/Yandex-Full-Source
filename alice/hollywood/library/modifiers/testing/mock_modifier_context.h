#pragma once

#include <alice/hollywood/library/modifiers/context/context.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NHollywood::NModifiers {

class TMockModifierContext : public IModifierContext {

public:
    TMockModifierContext() {
        ON_CALL(*this, Logger()).WillByDefault(testing::ReturnRef(TRTLogger::NullLogger()));
    }

    MOCK_METHOD(const TModifierFeatures&, GetFeatures, (), (const, override));
    MOCK_METHOD(const TModifierBaseRequest&, GetBaseRequest, (), (const, override));
    MOCK_METHOD(TRTLogger&, Logger, (), (override));
    MOCK_METHOD(bool, HasExpFlag, (TStringBuf), (const, override));
    MOCK_METHOD(const TExpFlags&, ExpFlags, (), (const, override));
    MOCK_METHOD(IRng&, Rng, (), (override));
    MOCK_METHOD(NMetrics::ISensors&, Sensors, (), (override));
};

} // namespace NAlice::NHollywood
