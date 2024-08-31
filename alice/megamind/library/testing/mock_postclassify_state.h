#pragma once

#include <alice/megamind/library/stage_wrappers/postclassify_state.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind {

class TMockPostClassifyState : public IPostClassifyState {
public:
    TMockPostClassifyState() {
        ON_CALL(*this, GetWinnerCombinator()).WillByDefault(testing::Return(Nothing()));
        ON_CALL(*this, GetContinueResponse()).WillByDefault(testing::Return(Nothing()));
    }
    MOCK_METHOD(TErrorOr<TMegamindAnalyticsInfo>, GetAnalytics, (), (override));
    MOCK_METHOD(TErrorOr<TQualityStorage>, GetQualityStorage, (), (override));
    MOCK_METHOD(TMaybe<NMegamindAppHost::TScenarioErrorsProto>, GetScenarioErrors, (), (override));
    MOCK_METHOD(TErrorOr<TString>, GetWinnerScenario, (), (override));
    MOCK_METHOD(TStatus, GetPostClassifyStatus, (), (override));
    MOCK_METHOD(TMaybe<TString>, GetWinnerCombinator, (), (override));
    MOCK_METHOD(TMaybe<NMegamindAppHost::TCombinatorProto::ECombinatorStage>, GetWinnerCombinatorStage, (), (override));
    MOCK_METHOD(TMaybe<NScenarios::TScenarioContinueResponse>, GetContinueResponse, (), (override));
};

} // namespace NAlice::NMegamind
