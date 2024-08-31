#include "prepare_teaser_settings_screen.h"

#include <alice/hollywood/library/combinators/combinators/centaur/teaser_service.h>
#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/protos/data/scenario/dialogovo/skill.pb.h>
#include <alice/protos/div/div2card.pb.h>


namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

namespace NMemento = ru::yandex::alice::memento::proto;

const TString TEASER_SETTINGS_ID = "teaser.settings.id";

TPrepareTeaserSettingsScreen::TPrepareTeaserSettingsScreen(THwServiceContext& ctx, TCombinatorRequestWrapper& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest),
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer)
{
}

void TPrepareTeaserSettingsScreen::Do() {
    
    LOG_INFO(Ctx.Logger()) << "Start preparing teaser settings screen";
    MakeScenarioResponse(GetCentaurTeasersDeviceConfig(CombinatorContextWrapper));

    TCombinatorUsedScenarios usedScenariosProto;
    for (const auto& usedScenario : UsedScenarios) {
        usedScenariosProto.AddScenarioNames(usedScenario);
    }
    Ctx.AddProtobufItemToApphostContext(usedScenariosProto, COMBINATOR_USED_SCENARIOS_ITEM);
}

void TPrepareTeaserSettingsScreen::MakeScenarioResponse(const NMemento::TCentaurTeasersDeviceConfig& centaurTeasersDeviceConfig) {
    
    LOG_INFO(Ctx.Logger()) << "Centaur Teasers combinator starts continue stage";
    
    const auto& runResponses = Request.GetScenarioRunResponses();
    const auto& continueResponses = Request.GetScenarioContinueResponses();
    if (runResponses.empty() && continueResponses.empty() ) {
        ResponseForRenderer.MutableFeatures()->SetIsIrrelevant(true);
        return;
    }

    THashMap<TString, NData::TTeasersPreviewData> teaserStack;

    for (const auto& scenarioResponses : runResponses) {
        const auto& scenarioName  = scenarioResponses.first;
        LOG_INFO(Ctx.Logger()) << "Found in run responses  " << scenarioName;
        const auto& scenarioResponseDirectives  = scenarioResponses.second.GetResponseBody().GetLayout().GetDirectives();
        if(scenarioResponses.second.GetResponseBody().GetScenarioData().HasTeasersPreviewData()) {
            const auto& scenarioResponseData = scenarioResponses.second.GetResponseBody().GetScenarioData().GetTeasersPreviewData();
            teaserStack[scenarioName] = scenarioResponseData;
        }
    }
    for (const auto& scenarioResponses : continueResponses) {
        const auto& scenarioName  = scenarioResponses.first;
        LOG_INFO(Ctx.Logger()) << "Found in continue responses  " << scenarioName;
        const auto& scenarioResponseDirectives  = scenarioResponses.second.GetResponseBody().GetLayout().GetDirectives();
        if(scenarioResponses.second.GetResponseBody().GetScenarioData().HasTeasersPreviewData()) {
            const auto& scenarioResponseData = scenarioResponses.second.GetResponseBody().GetScenarioData().GetTeasersPreviewData();
            teaserStack[scenarioName] = scenarioResponseData;
        }
    }

    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(TEASER_SETTINGS_ID);
    auto& teaserSettingsWithContentData = *divRendererData.MutableScenarioData()->MutableTeaserSettingsWithContentData();


    TVector<std::pair<TString, TString>> usedSettings;

    for(const auto& scenarioResponse: teaserStack) {
        LOG_INFO(Ctx.Logger()) << "Searching directives for scenario " << scenarioResponse.first;

        const auto& teaserPreviews = teaserStack[scenarioResponse.first].GetTeaserPreviews();
        
        for(const auto& teaserPreview: teaserPreviews) {
            NAlice::NData::TCentaurTeaserConfigData teaserConfigData;
            const auto& teaserType = teaserPreview.GetTeaserConfigData().GetTeaserType();
            const auto& teaserId = teaserPreview.GetTeaserConfigData().GetTeaserId();
            teaserConfigData.SetTeaserType(teaserType);
            teaserConfigData.SetTeaserId(teaserId);

            if(std::find(usedSettings.begin(), usedSettings.end(), std::make_pair(teaserType, teaserId)) == usedSettings.end()) {
                
                auto& data = *teaserSettingsWithContentData.AddTeaserSettingsWithPreview();

                data.SetIsChosen(CheckTeaserInSettings(centaurTeasersDeviceConfig, teaserConfigData));
                    
                if(!teaserPreview.GetTeaserName().Empty()) {
                     data.SetTeaserName(teaserPreview.GetTeaserName());
                }
                
                *data.MutableTeaserConfigData() = std::move(teaserConfigData);

                if(teaserPreview.HasTeaserPreviewScenarioData()) {
                    *data.MutableTeaserPreviewScenarioData() = std::move(teaserPreview.GetTeaserPreviewScenarioData());
                }

                usedSettings.emplace_back(teaserType, teaserId);
            } 
        }
    }

    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);

    auto& showViewDirective = *ResponseForRenderer.MutableResponseBody()
                                        ->MutableLayout()
                                        ->AddDirectives()
                                        ->MutableShowViewDirective();
    showViewDirective.SetName("show_view");
    showViewDirective.SetCardId(TEASER_SETTINGS_ID);
    showViewDirective.MutableLayer()->MutableDialog();
    showViewDirective.SetInactivityTimeout(TShowViewDirective_EInactivityTimeout_Infinity);
    showViewDirective.SetDoNotShowCloseButton(true);
    
    Ctx.AddProtobufItemToApphostContext(ResponseForRenderer, RESPONSE_ITEM);
}


} 
