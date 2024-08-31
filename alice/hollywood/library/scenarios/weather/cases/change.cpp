#include "change.h"

#include <alice/hollywood/library/scenarios/weather/util/util.h>

namespace NAlice::NHollywood::NWeather {

namespace {

static const TString ASKED_START_SLOT = "asked_start";
static const TString ASKED_END_SLOT = "asked_end";
static const TString PREC_START_SLOT = "prec_start";
static const TString PREC_END_SLOT = "prec_end";
static const TString PRECIPITATION_TYPE_SLOT = "precipitation_type";
static const TString WHEATHER_CONDITION_ASKED_SLOT = "weather_condition_asked";

static const TString DATE_SLOT_TYPE = "date";
static const TString PREC_CHANGE_SLOT_TYPE = "prec_change";
static const TString STRING_SLOT_TYPE = "string";

// int -> str mapping according to https://a.yandex-team.ru/arcadia/weather/libs/conditions/enums.h?rev=r9763225#L9
static const TVector<TStringBuf> PREC_TYPES = {
    "no_precipitation",
    "rain",
    "snow_with_rain",
    "snow",
    "hail",
};

bool MatchPrecType(const TString& precTypeName, const size_t precTypeValue) {
    return precTypeValue < PREC_TYPES.size() && PREC_TYPES[precTypeValue] == precTypeName
        || precTypeName == "precipitation" && precTypeValue > 0;
}

class TPrecipitationChangeFinder {
public:
    TPrecipitationChangeFinder(TWeatherContext& ctx)
        : Ctx(ctx)
        , Forecast(*Ctx.Forecast())
        , UserTime(Forecast.UserTime)
        , Now(TInstant::Seconds(Forecast.Now))
        , TenDays(Now + TDuration::Days(10))
    {}

public:
    void Find() {
        auto start = Now;
        auto end = TenDays;

        auto dtl = GetDateTimeList(Ctx, UserTime);
        if (const auto* dateTimeListPtr = std::get_if<std::unique_ptr<TDateTimeList>>(&dtl); (*dateTimeListPtr)->TotalDays() > 0) {
            const auto& dateTimeList = *dateTimeListPtr;
            if (const auto* askedStartDay = Forecast.FindDay(*dateTimeList->begin()); askedStartDay != nullptr && askedStartDay->Hours.begin()->HourTS > start) {
                start = askedStartDay->Hours.begin()->HourTS;

                NJson::TJsonValue askedStartSlot;
                askedStartSlot["date"] = askedStartDay->Date.ToString("%F");
                askedStartSlot["tz"] = UserTime.TimeZone().name();
                Ctx.AddSlot(ASKED_START_SLOT, DATE_SLOT_TYPE, askedStartSlot);
            }
            if (const auto* askedEndDay = Forecast.FindDay(*dateTimeList->rbegin()); askedEndDay != nullptr && askedEndDay->Hours.rbegin()->HourTS < end) {
                end = askedEndDay->Hours.rbegin()->HourTS;

                NJson::TJsonValue askedEndSlot;
                askedEndSlot["date"] = askedEndDay->Date.ToString("%F");
                askedEndSlot["tz"] = UserTime.TimeZone().name();
                Ctx.AddSlot(ASKED_END_SLOT, DATE_SLOT_TYPE, askedEndSlot);
            }
        }

        FindInRange(start, end);

        if (Ctx.IsSlotEmpty(PREC_START_SLOT) && !Ctx.IsSlotEmpty(PREC_END_SLOT)) {
            FindInRange(Now, end);
        } else if (!Ctx.IsSlotEmpty(PREC_START_SLOT) && Ctx.IsSlotEmpty(PREC_END_SLOT)) {
            FindInRange(start, TenDays);
        }
    }

private:
    void FindInRange(const TInstant& start, const TInstant& end) {
        const TString& askedWeatherCondition = Ctx.FindSlot(WHEATHER_CONDITION_ASKED_SLOT)->Value.AsString();
        auto prevPrecipitation = MatchPrecType(askedWeatherCondition, Forecast.Fact.PrecType);

        bool finished = false;
        for (const auto& day : Forecast.Days) {
            for (const auto& hour : day.Hours) {
                if (hour.HourTS < start) {
                    if (hour.HourTS > Now) {
                        prevPrecipitation = MatchPrecType(askedWeatherCondition, hour.PrecType);
                    }
                    continue;
                }
                if (hour.HourTS > end) {
                    finished = true;
                    break;
                }

                auto curPrecipitation = MatchPrecType(askedWeatherCondition, hour.PrecType);
                if (prevPrecipitation != curPrecipitation) {
                    Ctx.AddSlot(PRECIPITATION_TYPE_SLOT, STRING_SLOT_TYPE, askedWeatherCondition);

                    NJson::TJsonValue dateTimeSlot;
                    dateTimeSlot["date"] = day.Date.ToString("%F");
                    dateTimeSlot["tz"] = UserTime.TimeZone().name();
                    dateTimeSlot["hour"] = hour.Hour;
                    if (curPrecipitation) {
                        Ctx.AddSlot(PREC_START_SLOT, PREC_CHANGE_SLOT_TYPE, dateTimeSlot);
                    } else {
                        Ctx.AddSlot(PREC_END_SLOT, PREC_CHANGE_SLOT_TYPE, dateTimeSlot);
                        finished = true;
                        break;
                    }
                }
                prevPrecipitation = curPrecipitation;
            }
            if (finished) {
                break;
            }
        }  
    }

private:
    TWeatherContext& Ctx;
    const TForecast& Forecast;
    const TDateTime::TSplitTime& UserTime;
    const TInstant Now;
    const TInstant TenDays;
};

} // namespace

[[nodiscard]] TWeatherStatus PreparePrecipitationChangeSlots(TWeatherContext& ctx) {
    TPrecipitationChangeFinder(ctx).Find();

    ctx.Renderer().AddTextCard(NNlgTemplateNames::GET_WEATHER_CHANGE, "render_precipitation");

    TVector<ESuggestType> suggests{
        ESuggestType::Feedback,
        ESuggestType::SearchFallback,
        ESuggestType::Onboarding,
    };
    ctx.Renderer().AddSuggests(suggests);

    return EWeatherOkCode::RESPONSE_ALREADY_RENDERED;
}

} // namespace NAlice::NHollywood::NWeather
