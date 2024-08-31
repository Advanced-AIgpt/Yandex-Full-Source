#include "switch_weather.h"

namespace NAlice::NHollywood::NWeather {

TWeatherErrorOr<bool> IsGetWeatherScenario(const TWeatherContext& ctx) {
    NGeobase::TId geoId = ctx.WeatherPlace()->GetOriginalCityGeoId();
    NGeobase::TRegion region = ctx.GeobaseLookup().GetRegionById(geoId);
    if (region.GetEType() < NGeobase::ERegionType::CITY) {
        return true;
    }

    const auto& forecast = *ctx.Forecast();
    const auto userTime = forecast.UserTime;
    const auto dtl = GetDateTimeList(ctx, userTime);
    if (const auto err = std::get_if<TWeatherError>(&dtl)) {
        return *err;
    }

    const auto& dateTimeList = std::get<std::unique_ptr<TDateTimeList>>(dtl);

    auto userDateString = userTime.ToString("%F");
    if (!dateTimeList->IsNow() && dateTimeList->cbegin()->SplitTime().ToString("%F") != userDateString) { // not now and not today
        return true;
    }

    if (dateTimeList->TotalDays() > 1) { // date range
        return true;
    }
    return false;
}

} // namespace NAlice::NHollywood::NWeather
