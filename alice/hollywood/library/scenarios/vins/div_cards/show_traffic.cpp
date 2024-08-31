#include "show_traffic.h"

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/data/scenario/traffic/traffic.pb.h>

#include <alice/library/json/json.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NHollywoodFw::NVins {

namespace {

    NJson::TJsonValue GetDivCard(const NProtoVins::TBassResponse& response) {
        for (const auto& block : response.GetBlocks()) {
            if (block.GetType() == "div_card") {
                return JsonFromProto(block.GetData());
            }
        }
        return {};
    }
}

NData::TScenarioData TProcessorShowTraffic::Process(const NProtoVins::TVinsRunResponse& response) const {
    const auto& divCard = GetDivCard(response.GetBassResponse());
    if (!divCard.IsDefined()) {
        NData::TScenarioData renderCard;
        renderCard.MutableConversationData();
        return renderCard;
    }

    NData::TTrafficCardData trafficCard;
    for (const auto& slot : response.GetScenarioRunResponse().GetResponseBody().GetSemanticFrame().GetSlots()) {
        const auto& value = slot.GetTypedValue().GetString();
        if (value == "null") {
            continue;
        }
        if (slot.GetName() == "traffic_info") {
            auto trafficInfo = JsonFromString(value);
            trafficCard.SetCity(trafficInfo["city"].GetString());
            trafficCard.SetCityPrepcase(trafficInfo["city_prepcase"].GetString());
            trafficCard.SetInUserCity(trafficInfo["in_user_city"].GetBoolean());
            trafficCard.SetImageUrl(trafficInfo["static_map_url"].GetString());
            trafficCard.SetMapUrl(trafficInfo["url"].GetString());
            trafficCard.SetMessage(trafficInfo["hint"].GetString());
            trafficCard.SetLevel(trafficInfo["level"].GetString());
            trafficCard.SetIcon(trafficInfo["icon"].GetString());
            trafficCard.SetTrafficLightsImageUrl(divCard["data"]["traffic_lights_image_url"].GetString());
            for (const auto& forecastInfo : trafficInfo["forecast"].GetArray()) {
                auto* forecast = trafficCard.AddForecast();
                forecast->SetHour(forecastInfo["hour"].GetInteger());
                forecast->SetScore(forecastInfo["score"].GetInteger());
            }
        }
    }
    NData::TScenarioData renderCard;
    *renderCard.MutableTrafficCardData() = std::move(trafficCard);
    return renderCard;
}

}  // namespace NAlice::NHollywoodFw::NVins
