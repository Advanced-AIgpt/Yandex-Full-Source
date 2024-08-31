#pragma once

#include <alice/megamind/library/walker/requestctx.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

using namespace testing;

class TMockLightWalkerRequestCtx : virtual public ILightWalkerRequestCtx {
public:
    TMockLightWalkerRequestCtx() {
        static TScenarioConfigRegistry registry{};
        ON_CALL(*this, ScenarioConfigRegistry()).WillByDefault(ReturnRef(registry));
        ON_CALL(*this, RunStage()).WillByDefault(Return(ERunStage::Prepare));
    }

    // ILightWalkerRequestCtx
    MOCK_METHOD(IGlobalCtx&, GlobalCtx, (), ());
    MOCK_METHOD(const IGlobalCtx&, GlobalCtx, (), (const));
    MOCK_METHOD(TRequestCtx&, RequestCtx, (), (override));
    MOCK_METHOD(const TRequestCtx&, RequestCtx, (), (const, override));
    MOCK_METHOD(IContext&, Ctx, (), (override));
    MOCK_METHOD(const IContext&, Ctx, (), (const, override));
    MOCK_METHOD(TQualityStorage&, QualityStorage, (), ());
    MOCK_METHOD(IRng&, Rng, (), (override));
    MOCK_METHOD(ERunStage, RunStage, (), (const, override));
    MOCK_METHOD(NMegamind::TItemProxyAdapter&, ItemProxyAdapter, (), (override));
    MOCK_METHOD(const TScenarioConfigRegistry&, ScenarioConfigRegistry, (), ());
    MOCK_METHOD(NMegamind::IPostClassifyState&, PostClassifyState, (), (override));
    MOCK_METHOD(NMegamind::NModifiers::IModifierRequestFactory&, ModifierRequestFactory, (), (override));
};

class TMockRunWalkerRequestCtx : public IRunWalkerRequestCtx, public TMockLightWalkerRequestCtx {
public:
    // IRunWalkerRequestCtx
    MOCK_METHOD(TFactorStorage&, FactorStorage, (), (override));
    MOCK_METHOD(void, MakeProactivityRequest, (const TRequest&,
                                               const TScenarioToRequestFrames&,
                                               const NMegamind::TProactivityStorage&), (override));
    MOCK_METHOD(void, MakeSearchRequest, (TWebSearchRequestBuilder&, const IEvent&), (override));
    MOCK_METHOD(void, SavePostClassifyState, (const TWalkerResponse&,
                                              const NMegamind::TMegamindAnalyticsInfoBuilder&,
                                              TStatus,
                                              const TScenarioWrapperPtr,
                                              const TRequest&), (override));
};

class TMockApplyWalkerRequestCtx : public ILightWalkerRequestCtx {
public:
    TMockApplyWalkerRequestCtx() {
        static TScenarioConfigRegistry registry{};
        ON_CALL(*this, ScenarioConfigRegistry()).WillByDefault(ReturnRef(registry));
        ON_CALL(*this, RunStage()).WillByDefault(Return(ERunStage::Prepare));
    }

    // ILightWalkerRequestCtx
    MOCK_METHOD(IGlobalCtx&, GlobalCtx, (), ());
    MOCK_METHOD(const IGlobalCtx&, GlobalCtx, (), (const));
    MOCK_METHOD(TRequestCtx&, RequestCtx, (), (override));
    MOCK_METHOD(const TRequestCtx&, RequestCtx, (), (const, override));
    MOCK_METHOD(IContext&, Ctx, (), (override));
    MOCK_METHOD(const IContext&, Ctx, (), (const, override));
    MOCK_METHOD(TQualityStorage&, QualityStorage, (), ());
    MOCK_METHOD(IRng&, Rng, (), (override));
    MOCK_METHOD(ERunStage, RunStage, (), (const, override));
    MOCK_METHOD(NMegamind::TItemProxyAdapter&, ItemProxyAdapter, (), (override));
    MOCK_METHOD(const TScenarioConfigRegistry&, ScenarioConfigRegistry, (), ());
    MOCK_METHOD(NMegamind::IPostClassifyState&, PostClassifyState, (), (override));
    MOCK_METHOD(NMegamind::NModifiers::IModifierRequestFactory&, ModifierRequestFactory, (), (override));
};

} // namespace NAlice
