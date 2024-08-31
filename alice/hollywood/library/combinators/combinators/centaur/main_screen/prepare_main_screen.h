#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/widget_service.h>

namespace NAlice::NHollywood::NCombinators {

class TPrepareMainScreen {
public:
    TPrepareMainScreen(THwServiceContext& ctx, const NScenarios::TCombinatorRequest& combinatorRequest);
    void Do();

private:
    void SetMainScreenDirective();
    void SetUpperShutterDirective();
    void AddMyScreenTab(NScenarios::TSetMainScreenDirective& mainScreenDirective, const TMaybe<TMusicData>& musicData);
    void AddMusicTab(NScenarios::TSetMainScreenDirective& mainScreenDirective, const TMaybe<TMusicData>& musicData);
    void AddSmartHomeTab(NScenarios::TSetMainScreenDirective& mainScreenDirective);
    void AddServicesTab(NScenarios::TSetMainScreenDirective& mainScreenDirective);
    void AddDiscoveryTab(NScenarios::TSetMainScreenDirective& mainScreenDirective);
    const TMaybe<TMusicData> GetMusicScenarioData();
    TMaybe<TIoTUserInfo> GetIoTScenarioData() const;
    void FillMusicCardData(const auto& scenarioData, NData::TCentaurMainScreenGalleryMusicCardData& musicCardData);
    void PushMusicTabDataToContext(const TMusicData& musicData);
    void PushMusicCardDataToContext(const auto& scenarioData);
    void PushVideoCardDataToContext(const auto& scenarioData);
    void PushServicesCardDataToContext(const auto& scenarioData);
    void PushServicesTabDataToContext(const TVector<TServiceData>& servicesData);
    void PushUsedScenariosToContext();
    void AddUpdateMainScreenScheduledAction();

    THwServiceContext& Ctx;
    TCombinatorRequestWrapper Request;
    NScenarios::TScenarioRunResponse ResponseForRenderer;
    TCombinatorContextWrapper CombinatorContextWrapper;
    THashSet<TString> UsedScenarios;
    const TWidgetResponses WidgetResponses;
};

} // namespace NAlice::NHollywood::NCombinators
