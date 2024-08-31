#include "render_with_bass.h"
#include "renderer.h"

#include <alice/hollywood/library/bass_adapter/bass_adapter.h>
#include <alice/hollywood/library/bass_adapter/bass_renderer.h>
#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>

#include <alice/hollywood/library/scenarios/show_traffic_bass/names.h>
#include <alice/hollywood/library/scenarios/show_traffic_bass/proto/show_traffic_bass.pb.h>

#include <alice/protos/data/scenario/centaur/main_screen.pb.h>
#include <alice/protos/data/scenario/traffic/traffic.pb.h>
#include <alice/protos/data/scenario/data.pb.h>
#include <alice/library/json/json.h>


#include <util/string/join.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood {

namespace {

const TString MAIN_SCREEN_CARD_ID = "traffic.main_screen.div.card";

using TrafficInfoRef = TMaybe<std::reference_wrapper<NJson::TJsonValue>>;

TrafficInfoRef MutableTrafficInfo(NJson::TJsonValue& bassResponseBody) {
    for (auto& slot : bassResponseBody["form"]["slots"].GetArraySafe()) {
        if (slot["name"] == "traffic_info") {
            return std::ref(slot);
        }
    }
    return Nothing();
}

void AddForecastShort(TrafficInfoRef trafficInfoRef, TRTLogger& logger) {
    if (!trafficInfoRef.Defined()) {
        LOG_INFO(logger) << "No forecast";
        return;
    }

    auto& trafficInfo = trafficInfoRef->get();
    if (!trafficInfo.Has("value") || !trafficInfo["value"].Has("forecast")) {
        return;
    }

    const auto& forecast = trafficInfo["value"]["forecast"];
    if (forecast.GetArray().empty()) {
        return;
    }

    const i8 startLevel = trafficInfo["value"].Has("level")
                          ? FromString<i8>(trafficInfo["value"]["level"].GetString())
                          : forecast.GetArray()[0]["score"].GetInteger();
    const i8 maxRange = 1;
    i8 minLevel = startLevel;
    i8 maxLevel = startLevel;
    NJson::TJsonValue forecastShort;
    for (const auto& item : forecast.GetArray()) {
        const i8 currentLevel = item["score"].GetInteger();
        minLevel = Min(minLevel, currentLevel);
        maxLevel = Max(maxLevel, currentLevel);
        if (maxLevel - minLevel > maxRange) {
            forecastShort["hour"] = item["hour"].GetInteger();
            forecastShort["score"] = item["score"].GetInteger();
            if (currentLevel > startLevel) {
                forecastShort["type"] = "up";
            } else {
                forecastShort["type"] = "down";
            }
            break;
        }
    }
    if (!forecastShort.IsDefined()) {
        forecastShort["hour"] = forecast.GetArray().size();
        forecastShort["score"] = minLevel;
        forecastShort["type"] = "same";
    }
    trafficInfo["value"]["forecast_short"] = std::move(forecastShort);
}

void AddTrafficInfoToState(TrafficInfoRef trafficInfoRef, google::protobuf::Any* state, TRTLogger& logger) {
    if (!trafficInfoRef.Defined()) {
        return;
    }

    if (trafficInfoRef->get().Has("value") &&
        trafficInfoRef->get()["value"].Has("url")) {
        TTrafficInfo trafficInfo;
        trafficInfo.SetUrl(trafficInfoRef->get()["value"]["url"].GetString());
        state->PackFrom(std::move(trafficInfo));
    } else {
        LOG_ERROR(logger) << "TrafficInfo doesn't have url";
    }
}

void AddMainScreenData(
    TrafficInfoRef trafficInfoRef,
    NHollywood::TContext& ctx,
    NShowTrafficBass::TRenderer& renderer)
{
    if (!trafficInfoRef.Defined() || !trafficInfoRef->get().Has("value") ) {
        LOG_INFO(ctx.Logger()) << "No Traffic Data";
        return;
    }
    const auto traffic = trafficInfoRef->get()["value"];

    NRenderer::TDivRenderData renderData;
    renderData.SetCardId(MAIN_SCREEN_CARD_ID);

    auto& trafficMainScreenData = *renderData.MutableScenarioData()->MutableTrafficData();
    trafficMainScreenData.SetCity(traffic["city"].GetString());
    trafficMainScreenData.SetMessage(traffic["hint"].GetString());
    trafficMainScreenData.SetImageUrl(traffic["static_map_url"].GetString());
    trafficMainScreenData.SetMapUrl(traffic["url"].GetString());
    trafficMainScreenData.SetLevel(traffic["icon"].GetString());
    i32 score;
    if(!TryFromStringWithDefault<i32>(traffic["level"].GetString(), score, 0)) {
        LOG_ERR(ctx.Logger()) << "Traffic score = [" << traffic["level"].GetString() << "]";  
    } 
    trafficMainScreenData.SetScore(score);
    

    if (const auto* forecast = traffic.GetValueByPath("forecast")) {
        for (const auto& forecastJsonVal : forecast->GetArray()) {
            auto& forecastItem = *trafficMainScreenData.AddForecast();
            forecastItem.SetHour(forecastJsonVal["hour"].GetInteger());
            forecastItem.SetScore(forecastJsonVal["score"].GetInteger());
        }
    }
    renderer.AddScenarioData(renderData.GetScenarioData());
}

void AddWidgetData(
    TrafficInfoRef trafficInfoRef,
    NHollywood::TContext& ctx,
    NShowTrafficBass::TRenderer& renderer)
{
    if (!trafficInfoRef.Defined() || !trafficInfoRef->get().Has("value") ) {
        LOG_INFO(ctx.Logger()) << "No Traffic Data";
        return;
    }
    const auto traffic = trafficInfoRef->get()["value"];

    NData::TScenarioData scenarioData;
    auto& widgetData = *scenarioData.MutableCentaurScenarioWidgetData();
    widgetData.SetWidgetType("traffic");
    auto& cardData = *widgetData.AddWidgetCards();
    auto& trafficCardData = *cardData.MutableTrafficCardData();

    trafficCardData.SetCity(traffic["city"].GetString());
    trafficCardData.SetMessage(traffic["hint"].GetString());
    trafficCardData.SetImageUrl(traffic["static_map_url"].GetString());
    trafficCardData.SetMapUrl(traffic["url"].GetString());
    trafficCardData.SetLevel(traffic["icon"].GetString());
    i32 score;
    if(!TryFromStringWithDefault<i32>(traffic["level"].GetString(), score, 0)) {
        LOG_ERR(ctx.Logger()) << "Traffic score = [" << traffic["level"].GetString() << "]";  
    } 
    trafficCardData.SetScore(score);

    if (const auto* forecast = traffic.GetValueByPath("forecast")) {
        for (const auto& forecastJsonVal : forecast->GetArray()) {
            auto& forecastItem = *trafficCardData.AddForecast();
            forecastItem.SetHour(forecastJsonVal["hour"].GetInteger());
            forecastItem.SetScore(forecastJsonVal["score"].GetInteger());
        }
    }
    renderer.AddScenarioData(std::move(scenarioData));
}

} // namespace

namespace NImpl {

[[nodiscard]] std::unique_ptr<TScenarioRunResponse> BassShowTrafficRenderDoImpl(
    const TScenarioRunRequestWrapper& runRequest,
    NJson::TJsonValue& bassResponse,
    TContext& ctx,
    NShowTrafficBass::TRenderer& renderer)
{
    TBassResponseRenderer bassRenderer(runRequest, runRequest.Input(), renderer.GetBuilder(), ctx.Logger(),
                                       /* suggestAutoAction= */ false);
    if(bassResponse.Has("form") && bassResponse["form"].Has("name") && bassResponse["form"]["name"].GetString() == NShowTrafficBass::CENTAUR_COLLECT_MAIN_SCREEN_TRAFFIC_SEMANTIC_FRAME) {
        if (runRequest.HasExpFlag(NShowTrafficBass::SCENARIO_WIDGET_MECHANICS_EXP_FLAG_NAME)) {
            AddWidgetData(MutableTrafficInfo(bassResponse), ctx, renderer);
        } else {
            AddMainScreenData(MutableTrafficInfo(bassResponse), ctx, renderer);
        }
    } else {
        AddForecastShort(MutableTrafficInfo(bassResponse), ctx.Logger());
    }
    bassRenderer.Render(NShowTrafficBass::SHOW_TRAFFIC_BASS_NLG,
                        /* nlgPhraseName= */ "render_result",
                        bassResponse,
                        Default<TString>(),
                        Default<TString>(),
                        /* processSuggestsOnError= */ true);
    renderer.AddFeedBackSuggests();
    renderer.SetShouldListen(/* shouldListen= */ true);
    return std::move(renderer.GetBuilder()).BuildResponse();
}

} // namespace NImpl

void TBassShowTrafficRenderHandle::Do(TScenarioHandleContext& ctx) const {
    auto bassResponseBody = RetireBassRequest(ctx);

    const auto runRequest = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioRunRequestWrapper request(runRequest, ctx.ServiceCtx);
    NShowTrafficBass::TRenderer renderer{ctx, request};
    auto response = NImpl::BassShowTrafficRenderDoImpl(request, bassResponseBody, ctx.Ctx, renderer);
    AddTrafficInfoToState(MutableTrafficInfo(bassResponseBody),
                                 response->MutableResponseBody()->MutableState(),
                                 ctx.Ctx.Logger());
    ctx.ServiceCtx.AddProtobufItem(*response, RESPONSE_ITEM);
}

} // namespace NAlice::NHollywood
