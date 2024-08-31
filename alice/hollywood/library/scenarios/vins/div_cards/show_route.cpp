#include "show_route.h"

#include <alice/megamind/protos/common/frame.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/protos/data/scenario/route/route.pb.h>

#include <alice/library/json/json.h>

#include <google/protobuf/struct.pb.h>

namespace NAlice::NHollywoodFw::NVins {

namespace {

    void FillSlot(const NJson::TJsonValue& src, NData::TRoute::TSlot* dst) {
        dst->SetText(src["text"].GetString());
        dst->SetValue(src["value"].GetDouble());
    }

    void FillCarRoute(const NJson::TJsonValue& src, const TString& imageUri, NData::TRoute* dst) {
        dst->SetType(NData::TRoute::EType::TRoute_EType_CAR);
        dst->SetMapsUri(src["maps_uri"].GetString());
        dst->SetImageUri(imageUri);
        FillSlot(src["jams_time"], dst->MutableJamsTime());
        FillSlot(src["length"], dst->MutableLength());
        FillSlot(src["time"], dst->MutableTime());
    }

    void FillPedestrianRoute(const NJson::TJsonValue& src, const TString& imageUri, NData::TRoute* dst) {
        dst->SetType(NData::TRoute::EType::TRoute_EType_PEDESTRIAN);
        dst->SetMapsUri(src["maps_uri"].GetString());
        dst->SetImageUri(imageUri);
        FillSlot(src["walking_dist"], dst->MutableLength());
        FillSlot(src["time"], dst->MutableTime());
    }

    void FillPublicTransportRoute(const NJson::TJsonValue& src, const TString& imageUri, NData::TRoute* dst) {
        dst->SetType(NData::TRoute::EType::TRoute_EType_PUBLIC_TRANSPORT);
        dst->SetMapsUri(src["maps_uri"].GetString());
        dst->SetImageUri(imageUri);
        FillSlot(src["walking_dist"], dst->MutableLength());
        FillSlot(src["time"], dst->MutableTime());
        dst->SetTransfers(src["transfers"].GetUInteger());
    }

    void FillGeo(const NJson::TJsonValue& src, NData::TGeo* dst) {
        dst->SetAddressLine(src["address_line"].GetString());
        dst->SetCity(src["city"].GetString());
        dst->SetCityPrepcase(src["city_prepcase"].GetString());
        dst->SetCountry(src["country"].GetString());
        dst->SetGeoId(src["geoid"].GetUInteger());
        dst->SetHouse(src["house"].GetString());
        dst->SetInUserCity(src["in_user_city"].GetBoolean());
        dst->SetLevel(src["level"].GetString());
        dst->SetStreet(src["street"].GetString());
        auto* cases = dst->MutableCityCases();
        cases->SetDative(src["city_cases"]["dative"].GetString());
        cases->SetGenitive(src["city_cases"]["genitive"].GetString());
        cases->SetNominative(src["city_cases"]["nominative"].GetString());
        cases->SetPreposition(src["city_cases"]["preposition"].GetString());
        cases->SetPrepositional(src["city_cases"]["prepositional"].GetString());
    }

    void FillResolvedLocation(const TString& type, const TString& src, NData::TRouteLocation* dst) {
        auto value = JsonFromString(src);
        auto* resolvedLocation = dst->MutableResolvedLocation();
        resolvedLocation->SetGeoUri(value["geo_uri"].GetString());

        auto* location = resolvedLocation->MutableLocation();
        location->SetLat(value["location"]["lat"].GetDouble());
        location->SetLon(value["location"]["lon"].GetDouble());

        if (type == "geo") {
            FillGeo(value, resolvedLocation->MutableGeo());
        } else if (type == "poi") {
            FillGeo(value["geo"], resolvedLocation->MutableGeo());
            resolvedLocation->SetCompanyName(value["company_name"].GetString());
            resolvedLocation->SetName(value["name"].GetString());
            resolvedLocation->SetObjectCatalogPhotosUri(value["object_catalog_photos_uri"].GetString());
            resolvedLocation->SetObjectCatalogReviewsUri(value["object_catalog_reviews_uri"].GetString());
            resolvedLocation->SetObjectCatalogUri(value["object_catalog_uri"].GetString());
            resolvedLocation->SetObjectId(value["object_id"].GetString());
            resolvedLocation->SetObjectUri(value["object_uri"].GetString());
            resolvedLocation->SetPhone(value["phone"].GetString());
            resolvedLocation->SetPhoneUri(value["phone_uri"].GetString());
            resolvedLocation->SetUrl(value["url"].GetString());

            auto* hours = resolvedLocation->MutableHours();
            hours->SetCurrentStatus(value["hours"]["current_status"].GetString());
            hours->SetTimezone(value["hours"]["tz"].GetString());
            for (const auto& workingHoursSrc : value["hours"]["working"].GetArray()) {
                auto* workingHours = hours->AddWorking();
                workingHours->SetFrom(workingHoursSrc["from"].GetString());
                workingHours->SetTo(workingHoursSrc["to"].GetString());
            }
        }
    }

    void FillLocationType(const TString& type, const TString& src, NData::TRouteLocation* dst) {
        if (type == "named_location") {
            dst->SetNamedLocation(JsonFromString(src).GetString());
        }
    }

    NJson::TJsonValue GetDivCard(const NProtoVins::TBassResponse& response) {
        for (const auto& block : response.GetBlocks()) {
            if (block.GetType() == "div_card") {
                return JsonFromProto(block.GetData());
            }
        }
        return {};
    }
}

NData::TScenarioData TProcessorShowRoute::Process(const NProtoVins::TVinsRunResponse& response) const {
    const auto& divCard = GetDivCard(response.GetBassResponse());
    if (!divCard.IsDefined()) {
        NData::TScenarioData renderCard;
        renderCard.MutableConversationData();
        return renderCard;
    }

    NData::TShowRouteData routeCard;
    for (const auto& slot : response.GetScenarioRunResponse().GetResponseBody().GetSemanticFrame().GetSlots()) {
        const auto& type = slot.GetTypedValue().GetType();
        const auto& value = slot.GetTypedValue().GetString();
        if (value == "null") {
            continue;
        }
        if (slot.GetName() == "route_info") {
            auto routeInfo = JsonFromString(value);
            for (const auto& route : divCard["data"]["show_route_data"].GetArray()) {
                const auto& routeType = route["text_key"].GetString();
                const auto& imageUri = route["image_url"].GetString();
                if (routeType ==  "car") {
                    FillCarRoute(routeInfo[routeType], imageUri, routeCard.AddRoutes());
                } else if (routeType == "pedestrian") {
                    FillPedestrianRoute(routeInfo[routeType], imageUri, routeCard.AddRoutes());
                } else if (routeType == "public_transport") {
                    FillPublicTransportRoute(routeInfo[routeType], imageUri, routeCard.AddRoutes());
                }
            }
        } else if (slot.GetName() == "resolved_location_from") {
            FillResolvedLocation(type, value, routeCard.MutableFrom());
        } else if (slot.GetName() == "resolved_location_to") {
            FillResolvedLocation(type, value, routeCard.MutableTo());
        } else if (slot.GetName() == "resolved_location_via") {
            FillResolvedLocation(type, value, routeCard.MutableVia());
        } else if (slot.GetName() == "what_from") {
            FillLocationType(type, value, routeCard.MutableFrom());
        } else if (slot.GetName() == "what_to") {
            FillLocationType(type, value, routeCard.MutableTo());
        } else if (slot.GetName() == "what_via") {
            FillLocationType(type, value, routeCard.MutableVia());
        }
    }
    NData::TScenarioData renderCard;
    *renderCard.MutableShowRouteData() = std::move(routeCard);
    return renderCard;
}

}  // namespace NAlice::NHollywoodFw::NVins
