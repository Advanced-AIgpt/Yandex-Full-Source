#pragma once

#include "api.h"
#include <alice/bass/forms/geodb.h>
#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/smallgeo/region.h>

#include <alice/library/datetime/datetime.h>

#include <util/string/printf.h>

namespace NBASS::NWeather {

using TLatLon = std::pair<TString, TString>;

std::variant<std::unique_ptr<NAlice::TDateTimeList>, TError> GetDateTimeList(TContext& ctx, const NAlice::TDateTime::TSplitTime& userTime);

std::variant<TRequestedGeo, TError> GetCity(TContext& ctx);

std::variant<TLatLon, TError> GetLatLon(TContext& ctx);

TStringBuf GetForegroundType(i64 precType, double precStrength);

NSc::TValue GetHours(const THour& hour);

std::variant<NSc::TValue, TError> GetForecastLocationSlot(TContext& ctx);

// Returns requested city. If the attempt to get the city from the request fails, the function returns the most appropriate alternative.
TRequestedGeo CorrectGetRequestedCity(TContext& ctx);

void AddForecastLocationSlot(TContext& ctx, NSc::TValue forecastLocation);

std::variant<TString, TError> GetWeatherUrl(TContext& ctx, TMaybe<int> anchorDay = Nothing());
std::variant<TString, TError> GetWeatherMonthUrl(TContext& ctx, TMaybe<int> anchorDay = Nothing());

std::pair<int, int> GetMinMaxTempsFromDayParts(const TParts& parts);

void SetWeatherProductScenario(TContext& ctx);

void TryParseJsonFromAllSlots(TContext& ctx);

void AddLedDirective(TContext& ctx, const TVector<TString>& imageUris);

TVector<TString> MakeWeatherGifUris(const TContext& ctx, int temperature, size_t precType, double precStrength, double cloudiness);

} // namespace NBASS::NWeather
