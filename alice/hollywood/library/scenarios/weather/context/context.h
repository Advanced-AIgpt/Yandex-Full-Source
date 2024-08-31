#pragma once

#include <alice/hollywood/library/scenarios/weather/context/api.h>
#include <alice/hollywood/library/scenarios/weather/context/renderer.h>
#include <alice/hollywood/library/scenarios/weather/context/weather_protos.h>
#include <alice/hollywood/library/scenarios/weather/proto/weather.pb.h>
#include <alice/hollywood/library/scenarios/weather/util/avatars.h>
#include <alice/hollywood/library/scenarios/weather/util/error.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/capability_wrapper/capability_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/datetime/datetime.h>
#include <alice/library/geo/user_location.h>

namespace NAlice::NHollywood::NWeather {

/*
 * Wrapper class above Hollywood context.
 * Contains Weather-specific logic.
 */
class TWeatherContext : public TCapabilityWrapper<TScenarioRunRequestWrapper> {
public:
    TWeatherContext(TScenarioHandleContext& ctx, bool forecastReady = false);

    // wrappee context getters
    TScenarioHandleContext& Ctx() { return *Ctx_; }
    const TScenarioHandleContext& Ctx() const { return *Ctx_; }

    const TScenarioHandleContext* operator->() const { return Ctx_; }
    TScenarioHandleContext* operator->() { return Ctx_; }
    const TScenarioHandleContext& operator*() const { return *Ctx_; }
    TScenarioHandleContext& operator*() { return *Ctx_; }

    // request data functions
    TRTLogger& Logger() const { return Ctx_->Ctx.Logger(); }

    const TScenarioRunRequestWrapper& RunRequest() const;
    const NAlice::NScenarios::TCallbackDirective* GetCallback() const;
    const TWeatherState& WeatherState() const;

    const TUserLocation& UserLocation() const;
    const std::pair<double, double> GetLatLon() const;
    TInstant ClientTime() const;

    // third-party services
    const NGeobase::TLookup& GeobaseLookup() const;
    const TAvatarsMap& AvatarsMap() const;

    // weather API answers
    const TWeatherProtos& Protos() const;
    const std::unique_ptr<TForecast>& Forecast() const;
    const std::unique_ptr<TNowcast>& Nowcast() const;

    const TMaybe<TWeatherPlace>& WeatherPlace() const;

    // Alice answer renderer
    TRenderer& Renderer();

    // semantic frame functions
    const TMaybe<TFrame>& Frame() const;
    TMaybe<TFrame>& Frame();

    bool IsSlotEmpty(const TStringBuf name) const;
    TPtrWrapper<TSlot> FindOrAddSlot(const TString& name, const TString& type);
    TPtrWrapper<TSlot> FindSlot(const TString& name);
    TPtrWrapper<TSlot> FindSlot(const TString& name) const;
    void AddSlot(const TString& name, const TString& type, TString value);
    void AddSlot(const TString& name, const TString& type, const NJson::TJsonValue& value);
    void RemoveSlot(const TString& name);

    // error functions
    void AddError(const TWeatherError& error);
    TMaybe<TWeatherError> GetError(const TStringBuf itemName) const;

    // centaur
    bool IsCollectCardRequest() const;
    bool IsCollectMainScreenRequest() const;
    bool IsCollectWidgetGalleryRequest() const;
    bool IsCollectTeasersPreviewRequest() const;

    // capability wrapper
    bool SupportsCloudUi() const;

private:
    TScenarioHandleContext* Ctx_;
    const NScenarios::TScenarioRunRequest RunRequestProto_;
    TScenarioRunRequestWrapper RunRequest_;
    const TWeatherState WeatherState_;

    const TAvatarsMap AvatarsMap_;

    const TUserLocation UserLocation_;
    const TMaybe<TWeatherPlace> WeatherPlace_;

    TMaybe<TFrame> Frame_;
    const std::unique_ptr<TRenderer> Renderer_;
    const std::unique_ptr<TWeatherProtos> Protos_;
    const std::unique_ptr<TForecast> Forecast_;
    const std::unique_ptr<TNowcast> Nowcast_;
};

} // namespace NAlice::NHollywood::NWeather
