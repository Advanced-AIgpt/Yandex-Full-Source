#pragma once

#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NHollywood::NShowTrafficBass {

class TRenderer {
public:
    TRenderer(TScenarioHandleContext ctx, const TScenarioRunRequestWrapper& runRequest, const TFrame* frame = nullptr);

    google::protobuf::Any* GetMutableState();
    TRunResponseBuilder& GetBuilder();

    void SetProductScenarioName(const TString& scenarioName);
    void SetIntentName(const TString& intentName);
    void SetShouldListen(bool shouldListen);

    void AddFeedBackSuggests();
    void AddOpenUriDirective(const TString& uri, const TString& name);
    void AddTypeTextSuggest(TStringBuf type);
    void AddScenarioData(const NData::TScenarioData& scenarioData);

    void Render(TStringBuf templateName);
    void RenderFeedbackAnswer(const NScenarios::TCallbackDirective* callback);

private:
    TResponseBodyBuilder& BodyBuilder();
    NAlice::NScenarios::IAnalyticsInfoBuilder& AnalyticsInfoBuilder();

    void AddSpecialSuggest(TStringBuf type) ;
    void AddSpecialSuggestAction(TStringBuf name, TStringBuf type, TStringBuf data = TStringBuf()) ;

    TRTLogger& Logger;
    const TScenarioRunRequestWrapper& RunRequest;
    TNlgWrapper NlgWrapper;
    TRunResponseBuilder Builder;
    TNlgData NlgData;
    const TFrame* Frame;
};

} // namespace NAlice::NHollywood::NShowTrafficBass
