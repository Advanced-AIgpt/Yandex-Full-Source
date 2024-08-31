#include "set_teaser_settings.h"

#include <alice/library/proto/protobuf.h>

#include <alice/memento/proto/api.pb.h>
#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/memento/proto/device_configs.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/protos/data/scenario/centaur/teasers/teaser_settings.pb.h>


namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;

const TString EXP_FOR_TESTING = "show_saved_scenarios";

TSetTeaserSettings::TSetTeaserSettings(THwServiceContext& ctx, TCombinatorRequestWrapper& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest), 
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer)
{
}

void TSetTeaserSettings::Do(const TSemanticFrame& setTeaserConfigurationSemanticFrame) {
    LOG_INFO(Ctx.Logger()) << "Start setting teaser config for memento";
    TFrame frame = TFrame::FromProto(setTeaserConfigurationSemanticFrame);
    const auto scenarioForTeasersSlot = frame.FindSlot(SCENARIOS_FOR_TEASERS_DATA_SLOT);
    if (scenarioForTeasersSlot.IsValid()) {
        const auto& teaserSettingsData = setTeaserConfigurationSemanticFrame.GetTypedSemanticFrame().GetCentaurSetTeaserConfigurationSemanticFrame()
                                            .GetScenariosForTeasersSlot().GetTeaserSettingsData();

        NMemento::TCentaurTeasersDeviceConfig centaurTeasersDeviceConfig;
        TString res = "";
        for (auto const teaserSetting: teaserSettingsData.GetTeaserSettings()) {
            if(teaserSetting.GetIsChosen()) {
                res.append(teaserSetting.GetTeaserConfigData().GetTeaserType()).append(" ");
                *centaurTeasersDeviceConfig.AddTeaserConfigs() = std::move(teaserSetting.GetTeaserConfigData());
            }
        }
        
        UpdateMementoCentaurTeasersDeviceConfig(centaurTeasersDeviceConfig, CombinatorContextWrapper);

        *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives() = PrepareCollectCardsCallbackDirective();
        if (Request.HasExpFlag(EXP_FOR_TESTING)) {
            ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddCards()->SetText(TString("Сохранили настройки тизеров для сценариев: ").append(res));
        } else {
            ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddCards()->SetText(TString("Настройки для тизеров сохранены."));
        }
        ResponseForRenderer.MutableResponseBody()->MutableLayout()->SetOutputSpeech(" ");
        Ctx.AddProtobufItemToApphostContext(ResponseForRenderer, RESPONSE_ITEM);

    } else {
        ythrow yexception() << "Invalid data in semantic frame " << SET_TEASER_CONFIGURATION_FRAME_NAME << " (SET_TEASER_CONFIGURATION_FRAME_NAME).";
    }
}

TDirective TSetTeaserSettings::PrepareCollectCardsCallbackDirective() {
    TParsedUtterance payload;
    payload.MutableTypedSemanticFrame()->MutableCentaurCollectCardsSemanticFrame();
    auto& analytics = *payload.MutableAnalytics();
    analytics.SetProductScenario(CENTAUR_TEASERS_COMBINATOR_PSN);
    analytics.SetPurpose("refresh_teasers");
    analytics.SetOrigin(TAnalyticsTrackingModule_EOrigin_SmartSpeaker);

    TDirective directive;
    auto* callBackDirective = directive.MutableCallbackDirective();
    callBackDirective->SetName("@@mm_semantic_frame");
    callBackDirective->SetIgnoreAnswer(false);
    *callBackDirective->MutablePayload() = MessageToStruct(payload);
    return directive;
}

}
