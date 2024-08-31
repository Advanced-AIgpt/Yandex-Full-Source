#include "prepare_teasers.h"
#include <alice/hollywood/library/combinators/combinators/centaur/teaser_service.h>
#include <algorithm>

#include <alice/hollywood/library/combinators/combinators/centaur/defs.h>
#include <alice/hollywood/library/combinators/combinators/centaur/schedule_service.h>
#include <alice/hollywood/library/combinators/protos/used_scenarios.pb.h>
#include <alice/hollywood/library/hw_service_context/context.h>
#include <alice/megamind/protos/scenarios/action_space.pb.h>
#include <alice/megamind/protos/scenarios/combinator_request.pb.h>
#include <alice/protos/api/renderer/api.pb.h>
#include <alice/protos/data/scenario/centaur/teasers.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/hollywood/library/combinators/analytics_info/builder.h>
#include <alice/hollywood/library/combinators/metrics/names.h>

#include <alice/protos/data/scenario/weather/weather.pb.h>
#include <google/protobuf/wrappers.pb.h>

#include <apphost/lib/proto_answers/http.pb.h>


namespace NAlice::NHollywood::NCombinators {

using namespace NAlice::NScenarios;


inline bool Compare(TVector<TAddCardDirective>& param_1, TVector<TAddCardDirective>& param_2)
{
    return (param_1.size() < param_2.size());
};

TPrepareTeasers::TPrepareTeasers(THwServiceContext& ctx, TCombinatorRequestWrapper& combinatorRequest)
    : Ctx(ctx),
      Request(combinatorRequest),
      CombinatorContextWrapper(Ctx, Request, ResponseForRenderer)
{
}

void TPrepareTeasers::Do() {
    LOG_INFO(Ctx.Logger()) << "Start preparing teasers";
    if(Request.HasExpFlag(TEASER_SETTINGS_EXP_FLAG_NAME)) {
        LOG_DEBUG(Ctx.Logger()) << "Found experiment " << TEASER_SETTINGS_EXP_FLAG_NAME << " in request. Prepare teasers with new approach";
        MakeScenarioResponseWithSettings(GetCentaurTeasersDeviceConfig(CombinatorContextWrapper));
        Ctx.AddProtobufItemToApphostContext(std::move(ResponseForRenderer), RESPONSE_ITEM);
    } else {
        LOG_DEBUG(Ctx.Logger()) << "Did not find experiment " << TEASER_SETTINGS_EXP_FLAG_NAME << " in request. Prepare teasers with old approach";
        MakeScenarioResponse();
        Ctx.AddProtobufItemToApphostContext(std::move(ResponseForRenderer), RESPONSE_ITEM);
    }

    TCombinatorUsedScenarios usedScenariosProto;
    for (const auto& usedScenario : UsedScenarios) {
        usedScenariosProto.AddScenarioNames(usedScenario);
    }
    Ctx.AddProtobufItemToApphostContext(usedScenariosProto, COMBINATOR_USED_SCENARIOS_ITEM);
}

template <typename TScenarioResponse>
void TPrepareTeasers::AddScenarioResponse(
        const TScenarioResponse& scenarioResponse,
        THashMap<TString, std::pair<DirectivePointer, DirectivePointer>>& teaserStack, 
        const auto& scenarioName
) {
    const auto& directives = scenarioResponse->second.GetResponseBody().GetLayout().GetDirectives();
    teaserStack[scenarioName] = {directives.begin(), directives.end()};
    for (const auto& [id, actionSpace] : scenarioResponse->second.GetResponseBody().GetActionSpaces()) {
        (*ResponseForRenderer.MutableResponseBody()->MutableActionSpaces())[id] = actionSpace;
    }
}

void TPrepareTeasers::SetToRenderChromeLayers(const google::protobuf::Map<TString, NScenarios::TScenarioRunResponse>& scenarioResponses) {
    // EChromeLayerType.Default
    const auto& weatherScenarioResponse = scenarioResponses.find(WEATHER_SCENARIO_NAME);
    if (weatherScenarioResponse == scenarioResponses.end()) {
        LOG_ERROR(Ctx.Logger()) << "Can't find " << WEATHER_SCENARIO_NAME << " scenario response for chrome layer";
        return;
    }

    const auto& weatherScenarioData = weatherScenarioResponse->second.GetResponseBody().GetScenarioData();
    if(!weatherScenarioData.HasWeatherTeaserData()) {
        LOG_ERROR(Ctx.Logger()) << "Can't find WeatherTeaserData weather scenario response for chrome layer";
    }

    UsedScenarios.insert(WEATHER_SCENARIO_NAME);
    NRenderer::TDivRenderData divRendererData;
    divRendererData.SetCardId(DEFAULT_CHROME_LAYER_RENDER_KEY);
    const auto& weatherTeaserData = weatherScenarioData.GetWeatherTeaserData();
    divRendererData.MutableScenarioData()->MutableCentaurTeaserChromeDefaultLayerData()->MutableWeatherTeaserData()->CopyFrom(weatherTeaserData);
    Ctx.AddProtobufItemToApphostContext(divRendererData, RENDER_DATA_ITEM);
}


void TPrepareTeasers::MakeScenarioResponse() {
    LOG_INFO(Ctx.Logger()) << "Centaur Teasers combinator starts continue stage";
    const auto carouselId = GetCarouselId();
    const auto& runResponses = Request.GetScenarioRunResponses();
    const auto& continueResponses = Request.GetScenarioContinueResponses();
    if (runResponses.empty() && continueResponses.empty() ) {
        ResponseForRenderer.MutableFeatures()->SetIsIrrelevant(true);
        return;
    }

    THashMap<TString, std::pair<DirectivePointer, DirectivePointer>> teaserStack;
    const auto& teasers = Request.HasExpFlag(SKILLS_TEASER_EXP_FLAG_NAME) ? TEASER_SEQUENCE_WITH_SKILLS : TEASER_SEQUENCE;
    for (const auto& scenarioName : teasers) {
        if (!teaserStack.contains(scenarioName)) {
            const auto& scenarioContinueResponse = continueResponses.find(scenarioName);
            if (scenarioContinueResponse == continueResponses.end()) {
                const auto& scenarioRunResponse = runResponses.find(scenarioName);
                if (scenarioRunResponse == runResponses.end()) {
                    LOG_ERROR(Ctx.Logger()) << "Can't find " << scenarioName << " scenario response";
                    teaserStack[scenarioName] = {};
                    Ctx.Sensors().IncRate(NMetrics::LabelsCombinatorMissedScenarios("CentaurCombinator", scenarioName));
                    continue;
                } else {
                    AddScenarioResponse(scenarioRunResponse, teaserStack, scenarioName);
                }
            } else {
                AddScenarioResponse(scenarioContinueResponse, teaserStack, scenarioName);
            }
        }
        auto& [it, endIt] = teaserStack[scenarioName];
        while (it != endIt && !it->HasAddCardDirective()) {
            it++;
        }
        if (it == endIt) {
            continue;
        }
        UsedScenarios.insert(scenarioName);
        *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives() = std::move(TDirective(*it++));
    }

    TDirective directive;

    auto* rotateCardsDirective = directive.MutableRotateCardsDirective();
    if (carouselId.Defined()) {
        rotateCardsDirective->SetCarouselId(*carouselId);
    } else {
        rotateCardsDirective->SetCarouselId(Request.RequestId());
    }
    rotateCardsDirective->SetCarouselShowTimeSec(CAROUSEL_SHOW_TIME_SEC);

    *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives() =  std::move(directive);

    if (Request.HasExpFlag(CAROUSEL_SERVER_UPDATE_EXP_FLAG_NAME)) {
        const auto& clientInfoProto = Request.BaseRequestProto().GetClientInfo();
        auto updateAction = CreateUpdateCarouselScheduleAction(clientInfoProto, Request.BlackBoxUserInfo(), Ctx.Logger());
        if (updateAction.Defined()) {
            LOG_INFO(Ctx.Logger()) << "Adding scheduled update action for centaur_collect_cards";
            *ResponseForRenderer.MutableResponseBody()->AddServerDirectives() = std::move(*updateAction);
        }
    }

    SetToRenderChromeLayers(runResponses);
}


void TPrepareTeasers::MakeScenarioResponseWithSettings(const NMemento::TCentaurTeasersDeviceConfig& centaurTeasersDeviceConfig) {
    LOG_INFO(Ctx.Logger()) << "Centaur Teasers combinator starts continue stage";
    const auto carouselId = GetCarouselId();
    const auto& runResponses = Request.GetScenarioRunResponses();
    const auto& continueResponses = Request.GetScenarioContinueResponses();
    if (runResponses.empty() && continueResponses.empty() ) {
        ResponseForRenderer.MutableFeatures()->SetIsIrrelevant(true);
        return;
    }

    THashMap<TString, std::pair<DirectivePointer, DirectivePointer>> teaserStack;
    for(const auto& scenarioResponses : runResponses) {
        const TString scenarioName  = scenarioResponses.first;
        const auto& scenarioResponseDirectives  = scenarioResponses.second.GetResponseBody().GetLayout().GetDirectives();
        teaserStack[scenarioName] = {scenarioResponseDirectives.begin(), scenarioResponseDirectives.end()};
    }
    for(const auto& scenarioResponses : continueResponses) {
        const TString scenarioName  = scenarioResponses.first;
        const auto& scenarioResponseDirectives  = scenarioResponses.second.GetResponseBody().GetLayout().GetDirectives();
        teaserStack[scenarioName] = {scenarioResponseDirectives.begin(), scenarioResponseDirectives.end()};
    }
    
    THashMap<TString, TVector<TAddCardDirective>> addCardDirectivesStack;
    for(auto& [scenarioName, scenarioDirectives]: teaserStack) {
        LOG_DEBUG(Ctx.Logger()) << "Searching directives for scenario " << scenarioName;
        auto& [it, endIt] =  scenarioDirectives;
        while (it != endIt) {
            if(it->HasAddCardDirective()) {
                const auto addCardDirective = it->GetAddCardDirective();
                const auto teaserType = addCardDirective.GetTeaserConfig().GetTeaserType();
                const auto teaserId = addCardDirective.GetTeaserConfig().GetTeaserId();

                NAlice::NData::TCentaurTeaserConfigData teaserConfigData;
                teaserConfigData.SetTeaserType(teaserType);
                teaserConfigData.SetTeaserId(teaserId);
                if(CheckTeaserInSettings(centaurTeasersDeviceConfig, teaserConfigData)) {
                    auto directivesForTeaserType = addCardDirectivesStack.find(teaserType);
                    if (directivesForTeaserType != addCardDirectivesStack.end()) {
                        addCardDirectivesStack[teaserType].push_back(addCardDirective);
                    } else {
                        addCardDirectivesStack.insert(std::make_pair(teaserType,TVector{addCardDirective}));
                    }
                }
            }
            ++it;
        }
    }

    TVector<int> sizeVector;
    TVector<TVector<TAddCardDirective>> elementVector;
    TVector<AddDirectivePointer> pointers;
    for(const auto& teasers: addCardDirectivesStack) {
        sizeVector.push_back(teasers.second.size());
        elementVector.push_back(teasers.second);
        pointers.push_back(teasers.second.begin());
    }
    sort(elementVector.begin(), elementVector.end(), Compare);
    TVector<TAddCardDirective> res = getSortedTeasers(sizeVector, pointers); 

    for(const auto& directive: res) {
        *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives()->MutableAddCardDirective() = directive;
    }

    TDirective directive;

    auto* rotateCardsDirective = directive.MutableRotateCardsDirective();
    if (carouselId.Defined()) {
        rotateCardsDirective->SetCarouselId(*carouselId);
    } else {
        rotateCardsDirective->SetCarouselId(Request.RequestId());
    }
    rotateCardsDirective->SetCarouselShowTimeSec(CAROUSEL_SHOW_TIME_SEC);

    *ResponseForRenderer.MutableResponseBody()->MutableLayout()->AddDirectives() =  std::move(directive);

    if (Request.HasExpFlag(CAROUSEL_SERVER_UPDATE_EXP_FLAG_NAME)) {
        const auto& clientInfoProto = Request.BaseRequestProto().GetClientInfo();
        auto updateAction = CreateUpdateCarouselScheduleAction(clientInfoProto, Request.BlackBoxUserInfo(), Ctx.Logger());
        if (updateAction.Defined()) {
            LOG_INFO(Ctx.Logger()) << "Adding scheduled update action for centaur_collect_cards";
            *ResponseForRenderer.MutableResponseBody()->AddServerDirectives() = std::move(*updateAction);
        }
    }

    SetToRenderChromeLayers(runResponses);
}

TVector<TAddCardDirective> TPrepareTeasers::getSortedTeasers(
    TVector<int> sizeVector, 
    TVector<AddDirectivePointer> pointers 
) {

    auto normName = std::accumulate(sizeVector.begin(), sizeVector.end(), 0);
    if(!normName) {
        return {};
    }

    TVector<TAddCardDirective> resVector;
    if (sizeVector.size() != 0 && normName == sizeVector[0]) {
        for (int i = 0; i < sizeVector[0]; ++i) {
            resVector.push_back(*pointers[0]);
            ++pointers[0];
        }
        return resVector;
    }

    TVector<AddDirectivePointer> pointersLeft;
    TVector<AddDirectivePointer> pointersRight;
    for(int i = 0; i < sizeVector.size(); ++i) {

        if(sizeVector[i] % 2 != 0) {
            resVector.push_back(*pointers[i]);
            --sizeVector[i];
            ++pointers[i];
        }
        
        sizeVector[i] /= 2;
        pointersLeft.push_back(pointers[i]);
        pointersRight.push_back(pointers[i] + sizeVector[i]);

    }
    auto leftVector = getSortedTeasers(sizeVector, pointersLeft);
    auto rightVector = getSortedTeasers(sizeVector, pointersRight);
    leftVector.insert(leftVector.end(), resVector.begin(), resVector.end());
    leftVector.insert(leftVector.end(), rightVector.begin(), rightVector.end());
    return leftVector;
}


TMaybe<TString> TPrepareTeasers::GetCarouselId() {
    if (const auto collectCardFrameProto = Request.Input().FindSemanticFrame(COLLECT_CARDS_FRAME_NAME)) {
        const auto collectCardFrame = TFrame::FromProto(*collectCardFrameProto);
        if (const auto carouselSlot = collectCardFrame.FindSlot(CAROUSEL_ID_SLOT_NAME)) {
            return carouselSlot->Value.AsString();
        }
    }
    return Nothing();
}

}
