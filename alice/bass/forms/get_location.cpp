#include "get_location.h"
#include "directives.h"

#include <alice/bass/libs/config/config.h>

#include <alice/bass/forms/geocoder.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/geoaddr.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/forms/navigator/navigator_intent.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/maps_static_api.h>
#include <alice/bass/libs/client/experimental_flags.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/datetime/base.h>

namespace NBASS {

namespace {

constexpr ui16 CARD_MAP_IMAGE_WIDTH = 520;
constexpr ui16 CARD_MAP_IMAGE_HEIGHT = 320;
constexpr ui16 CARD_MAP_USER_CENTERED_IMAGE_ZOOM = 17;


constexpr TDuration LOCATION_RECENCY_THRESHOLD = TDuration::Hours(1); // 1 hour
constexpr double LOCATION_ACCURACY_HOUSE_THRESHOLD = 200.0; // 200m
constexpr double LOCATION_ACCURACY_DISTRICT_THRESHOLD = 3000.0; // 3km
constexpr double LOCATION_ACCURACY_TOWN_THRESHOLD = 5000.0; // 5km

constexpr TStringBuf GEOCODER_DISTRICT_LEVEL_ZOOM = "12";
constexpr TStringBuf GEOCODER_TOWN_LEVEL_ZOOM = "9";
constexpr TStringBuf GEOCODER_SOMEWHERE_LEVEL_ZOOM = "6";

class TShowUserPositionNavigatorIntent : public INavigatorIntent {
    public:
        TShowUserPositionNavigatorIntent(TContext& ctx)
            : INavigatorIntent(ctx, TStringBuf("show_user_position") /* scheme */)
        {}

    private:
        TResultValue SetupSchemeAndParams() override {
            return TResultValue();
        }

        TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
            return GetAnalyticsTagIndex<TNavigatorShowPositionDirective>();
        }
};

} // end anon namespace

TString GenerateStaticMapUri(TContext& ctx) {
    NMapsStaticApi::TImageUrlBuilder urlBuilder{ctx};
    urlBuilder.SetCenter(ctx.Meta().Location().Lon(), ctx.Meta().Location().Lat()).SetZoom(
                CARD_MAP_USER_CENTERED_IMAGE_ZOOM);
    urlBuilder.SetSize(CARD_MAP_IMAGE_WIDTH, CARD_MAP_IMAGE_HEIGHT).Set(TStringBuf("scale"), "1.1");
    double my_lon = ctx.Meta().Location().Lon();
    double my_lat = ctx.Meta().Location().Lat();
    urlBuilder.AddPoint(my_lon, my_lat, "ya_ru");
    return urlBuilder.Build();
}

TResultValue TGetMyLocationHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::GET_MY_LOCATION);
    if (HasActualLocation(r.Ctx())) {
        TGeoPosition userLocation;
        userLocation.Lat = r.Ctx().Meta().Location().Lat();
        userLocation.Lon = r.Ctx().Meta().Location().Lon();

        NSc::TValue resolvedLocation;
        std::unique_ptr<TGeoObjectResolver> resolver =
            std::make_unique<TGeoObjectResolver>(r.Ctx(), userLocation, GetLocationZoom(r.Ctx()));

        if (TResultValue geoError = resolver->WaitAndParseResponse(&resolvedLocation)) {
            return geoError;
        }

        if (resolvedLocation.IsNull()) {
            ProcessError(r.Ctx(), TError::EType::NOGEOFOUND, TStringBuf("no geo found for user location"));
            AddSuggests(r.Ctx());
            return TResultValue();
        }
        ProcessLocation(r.Ctx(), resolvedLocation);
    } else {
        ProcessError(r.Ctx(), TError::EType::NOUSERGEO, TStringBuf("no user location"));
    }

    if (r.Ctx().ClientFeatures().SupportsNavigator()) {
        TShowUserPositionNavigatorIntent navigatorIntent(r.Ctx());
        return navigatorIntent.Do();
    }

    AddSuggests(r.Ctx());
    return TResultValue();
}

void TGetMyLocationHandler::Register(THandlersMap* handlers) {
    auto cbGetMyLocationForm = []() {
        return MakeHolder<TGetMyLocationHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.get_my_location"), cbGetMyLocationForm);
}

bool TGetMyLocationHandler::HasActualLocation(const TContext& ctx) const {
    if (!ctx.Meta().HasLocation()) {
        return false;
    }

    const i64 recency = ctx.Meta().Location().Recency();
    return (recency >= 0) && (static_cast<ui64>(recency) < LOCATION_RECENCY_THRESHOLD.MilliSeconds());
}


TStringBuf TGetMyLocationHandler::GetLocationZoom(const TContext& ctx) const {
    const double accuracy = ctx.Meta().Location().Accuracy();
    // will retrieve 'exact' location when no zoom specified
    if (accuracy < LOCATION_ACCURACY_HOUSE_THRESHOLD) {
        return "";
    }
    if (accuracy < LOCATION_ACCURACY_DISTRICT_THRESHOLD) {
        return GEOCODER_DISTRICT_LEVEL_ZOOM;
    }
    if (accuracy < LOCATION_ACCURACY_TOWN_THRESHOLD) {
        return GEOCODER_TOWN_LEVEL_ZOOM;
    }

    return GEOCODER_SOMEWHERE_LEVEL_ZOOM;
}

void TGetMyLocationHandler::ProcessError(TContext& ctx, TError::EType errorType, const TStringBuf& errorMessage) {
    ctx.AddErrorBlock(errorType, errorMessage);
    NSc::TValue mapsGeoUri;
    mapsGeoUri["geo_uri"] = GenerateMapsUri(ctx, TCgiParameters());
    ProcessLocation(ctx, mapsGeoUri);
}

void TGetMyLocationHandler::ProcessLocation(TContext& ctx, const NSc::TValue& location) {
    ctx.CreateSlot(TStringBuf("location"), TStringBuf("geo"), true, std::move(location));
    if (ctx.HasExpFlag(EXPERIMENTAL_FLAG_MY_LOCATION) && ctx.ClientFeatures().SupportsDivCards()) {
        NSc::TValue data;
        data["location_url"] = GenerateStaticMapUri(ctx);
        ctx.AddDivCardBlock(TStringBuf("my_location_map"),std::move(data));
        ctx.AddTextCardBlock(TStringBuf("my_location_map"), {});
    }
}

void TGetMyLocationHandler::AddSuggests(TContext& ctx) {
    if (!ctx.ClientFeatures().SupportsNavigator()) {
        if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_MY_LOCATION) || !ctx.ClientFeatures().SupportsDivCards()) {
            ctx.AddSuggest(TStringBuf("get_my_location__show_on_map"));
        }
        ctx.AddSearchSuggest();
        ctx.AddOnboardingSuggest();
     }
}

}
