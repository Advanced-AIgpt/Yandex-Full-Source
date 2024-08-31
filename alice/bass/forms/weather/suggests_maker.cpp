#include "suggests_maker.h"
#include "consts.h"

#include <util/generic/algorithm.h>

namespace NBASS::NWeather {

enum EPosition { Begin, End };

void MoveElement(const ESuggestType element, TVector<ESuggestType>& suggests, const EPosition position,
                 const bool skipNonExisting = false) {
    const auto it = Find(suggests, element);
    const bool exist = it != suggests.end();
    if (exist) {
        suggests.erase(it);
    }
    if (exist || !skipNonExisting) {
        suggests.insert(position == EPosition::Begin ? suggests.begin() : suggests.end(), element);
    }
}

void AlterSuggestsForExperiments(const TContext& ctx, TVector<ESuggestType>& suggests) {
    // See https://st.yandex-team.ru/WEATHER-15572#5e2012e0c87811669e29999f
    const bool isNewSuggests =
        ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_3) || ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_4);

    const bool shouldAddNowcastSuggest =
        ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_2) || ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_7);

    // We suggest that the suggests come in the new form. We remove all the new elements from them if the exps require
    // the old form

    if (!isNewSuggests && IsIn(suggests, ESuggestType::Tomorrow)) {
        if (const auto it = Find(suggests, ESuggestType::AfterTomorrow); it != suggests.end()) {
            suggests.erase(it);
        }
    }

    if (!isNewSuggests && !shouldAddNowcastSuggest) {
        if (const auto it = Find(suggests, ESuggestType::NowcastWhenStarts); it != suggests.end()) {
            suggests.erase(it);
        }

        if (const auto it = Find(suggests, ESuggestType::NowcastWhenEnds); it != suggests.end()) {
            suggests.erase(it);
        }
    }

    // It catches the tricky "погода на следующие выходные" case for removing the search suggest
    const bool isNextWeekend = IsIn(suggests, ESuggestType::Today) && IsIn(suggests, ESuggestType::Tomorrow);
    if (!isNewSuggests && isNextWeekend) {
        if (const auto it = Find(suggests, ESuggestType::SearchFallback); it != suggests.end()) {
            suggests.erase(it);
        }
    }

    // Alter the suggests for each experiment

    if (ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_1)) {
        MoveElement(ESuggestType::Onboarding, suggests, EPosition::Begin);
        return;
    }

    if (ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_2)) {
        // Do nothing, already preserved the new suggest above (`shouldAddNowcastSuggest`)
        return;
    }

    if (ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_3)) {
        MoveElement(ESuggestType::SearchFallback, suggests, EPosition::End);
        return;
    }

    if (ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_4)) {
        MoveElement(ESuggestType::Feedback, suggests, EPosition::End);
        return;
    }

    if (ctx.HasExpFlag(NExperiment::WEATHER_SUGGEST_5)) {
        MoveElement(ESuggestType::Feedback, suggests, EPosition::End);

        MoveElement(ESuggestType::SearchFallback, suggests, EPosition::Begin);
        return;
    }
}

void WriteSuggests(TContext& ctx, TVector<ESuggestType> suggests, const NAlice::TDateTime::TSplitTime* time,
                   const NSc::TValue* weatherJson) {
    if (!IsIn(suggests, ESuggestType::Onboarding)) {
        suggests.push_back(ESuggestType::Onboarding);
    }

    // For https://st.yandex-team.ru/WEATHER-15572
    AlterSuggestsForExperiments(ctx, suggests);

    for (ESuggestType st : suggests) {
        switch (st) {
            case ESuggestType::Verbose:
                if (time && weatherJson) {
                    NSc::TValue action;
                    action["url"].SetString(GenerateWeatherUri(
                        ctx.MetaClientInfo(), (*weatherJson)["info"]["url"].GetString(), time->MDay()));
                    ctx.AddSuggest(TStringBuf("forecast_verbose"), std::move(action));
                }
                break;
            case ESuggestType::Today:
                ctx.AddSuggest(TStringBuf("forecast_today"));
                break;
            case ESuggestType::Tomorrow:
                ctx.AddSuggest(TStringBuf("forecast_tomorrow"));
                break;
            case ESuggestType::AfterTomorrow:
                ctx.AddSuggest(TStringBuf("forecast_aftertomorrow"));
                break;
            case ESuggestType::Weekend:
                ctx.AddSuggest(TStringBuf("forecast_weekend"));
                break;
            case ESuggestType::SearchFallback:
                ctx.AddSearchSuggest();
                break;
            case ESuggestType::NowcastWhenStarts:
                ctx.AddSuggest(TStringBuf("nowcast_when_starts"));
                break;
            case ESuggestType::NowcastWhenEnds:
                ctx.AddSuggest(TStringBuf("nowcast_when_ends"));
                break;
            case ESuggestType::Onboarding:
                ctx.AddOnboardingSuggest();
                break;
            case ESuggestType::Feedback:
                // TODO: Find how to move the feedback block
                break;
        }
    }
}

} // namespace NBASS::NWeather
