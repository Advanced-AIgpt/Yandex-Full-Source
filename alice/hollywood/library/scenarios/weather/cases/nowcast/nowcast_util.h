#pragma once

#include <alice/hollywood/library/scenarios/weather/context/context.h>

namespace NAlice::NHollywood::NWeather {

NJson::TJsonValue MakeDivCard(const TWeatherContext& ctx);
void AddNowcastDivCardBlock(TWeatherContext& ctx, NJson::TJsonValue& divCard);
TString MakeNowcastNowUrl(const TWeatherContext& ctx);
TString ConstructNowcastUrl(const TWeatherContext& ctx, TString startUrl, bool turboInsertEnable = true);
void SuggestNowcastNowUrlSlot(TWeatherContext& ctx);
void SuggestHomeUrlSlot(TWeatherContext &ctx);
bool IsTurbo(const TWeatherContext& ctx);
void PrepareWeatherOpenUriSlot(TWeatherContext& ctx, const TString& uri);

} // namespace NAlice::NHollywood::NWeather
