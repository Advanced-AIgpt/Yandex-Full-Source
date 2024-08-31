#pragma once

#include <alice/megamind/library/scenarios/helpers/interface/scenario_wrapper.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

class TMockScenarioWrapper : public IScenarioWrapper {
public:
    TMockScenarioWrapper() {
        ON_CALL(*this, IsApplyNeededOnWarmUpRequestWithSemanticFrame()).WillByDefault(testing::Return(false));
    }

    MOCK_METHOD(void, Accept, (const IScenarioVisitor&), (const, override));
    MOCK_METHOD(const TScenario&, GetScenario, (), (const, override));
    MOCK_METHOD(TLightScenarioEnv, GetApplyEnv, (const TRequest&, const IContext&), (override));
    MOCK_METHOD(TScenarioEnv, GetEnv, (const TRequest&, const IContext&), (override));
    MOCK_METHOD(const TSemanticFrames&, GetSemanticFrames, (), (const, override));
    MOCK_METHOD(NMegamind::TAnalyticsInfoBuilder&, GetAnalyticsInfo, (), (override));
    MOCK_METHOD(const NMegamind::TAnalyticsInfoBuilder&, GetAnalyticsInfo, (), (const, override));
    MOCK_METHOD(NMegamind::TUserInfoBuilder&, GetUserInfo, (), (override));
    MOCK_METHOD(const NMegamind::TUserInfoBuilder&, GetUserInfo, (), (const, override));
    MOCK_METHOD(NMegamind::TMegamindAnalyticsInfo&, GetMegamindAnalyticsInfo, (), (override));
    MOCK_METHOD(TQualityStorage&, GetQualityStorage, (), (override));
    MOCK_METHOD(const TQualityStorage&, GetQualityStorage, (), (const, override));
    MOCK_METHOD(NMegamind::TModifiersStorage&, GetModifiersStorage, (), (override));
    MOCK_METHOD(const NMegamind::TModifiersStorage&, GetModifiersStorage, (), (const, override));
    MOCK_METHOD(TStatus, Init, (const TRequest&, const IContext&, NMegamind::IDataSources&), (override));
    MOCK_METHOD(TStatus, Ask, (const TRequest&, const IContext&, TScenarioResponse&), (override));
    MOCK_METHOD(TStatus, Finalize, (const TRequest&, const IContext&, TScenarioResponse&), (override));
    MOCK_METHOD(std::once_flag&, GetAskFlag, (), (override));
    MOCK_METHOD(TStatus, StartHeavyContinue, (const TRequest&, const IContext&), (override));
    MOCK_METHOD(TStatus, FinishContinue, (const TRequest&, const IContext&, TScenarioResponse&), (override));
    MOCK_METHOD(std::once_flag&, GetContinueFlag, (), (override));
    MOCK_METHOD(EApplicability, SetReasonWhenNonApplicable, (const TRequest&, const IContext&, TScenarioResponse&), (override));
    MOCK_METHOD(TErrorOr<EApplyResult>, StartApply, (const TRequest&, const IContext&, TScenarioResponse&, const NMegamind::TMegamindAnalyticsInfo&, const TQualityStorage&, const TProactivityAnswer&), (override));
    MOCK_METHOD(TErrorOr<EApplyResult>, FinishApply, (const TRequest&, const IContext&, TScenarioResponse&), (override));
    MOCK_METHOD(bool, IsSuccess, (), (const, override));
    MOCK_METHOD(bool, ShouldBecomeActiveScenario, (), (const, override));
    MOCK_METHOD(EDeferredApplyMode, GetDeferredApplyMode, (), (const, override));
    MOCK_METHOD(NMegamindAppHost::TScenarioProto, GetScenarioProto, (), (const, override));
    MOCK_METHOD(TStatus, RestoreInit, (NMegamind::TItemProxyAdapter& itemAdapter), (override));
    MOCK_METHOD(bool, IsApplyNeededOnWarmUpRequestWithSemanticFrame, (), (const, override));
};

} // namespace NAlice
