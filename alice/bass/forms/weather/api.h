#pragma once

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/util/error.h>

#include <alice/library/datetime/datetime.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/civil.h>

#include <util/generic/variant.h>

namespace NBASS::NWeather {

TString CloudinessPrecCssStyle(const double cloudness, const double precStrength);
TString MakeIconUrl(const TString& icon, const size_t size = 128, const TString& theme = "light");

class TTzInfo {
public:
    TString Name;
    int Offset;

public:
    TTzInfo(const NSc::TValue& tzInfoJson)
        : Name(tzInfoJson["name"])
        , Offset(tzInfoJson["offset"]) {
    }

    TString TimeZoneName() const {
        if (!Name.StartsWith("UTC")) {
            return Name;
        }

        return TStringBuilder()
            << "Etc/GMT"
            << (Offset > 0 ? "-" : "+")
            << (std::abs(Offset) / 60 / 60);
    }
};

class TInfo {
public:
    TTzInfo TzInfo;

public:
    TInfo(const NSc::TValue& infoSection)
        : TzInfo(infoSection["tzinfo"]) {
    }
};

class TGeoObject {
public:
    TGeoObject(const NSc::TValue& geoObjectSection) {
        Y_UNUSED(geoObjectSection);
    }
};

class TYesterday {
public:
    double Temp;

public:
    TYesterday(const NSc::TValue& yesterdayJson)
        : Temp(yesterdayJson["temp"].ForceNumber()) {
    }
};

class TFact {
public:
    double Cloudness;
    double PrecStrength;
    double Temp;
    double FeelsLike;
    size_t PrecType;

    TString Icon;
    TString Condition;

public:
    TFact(const NSc::TValue& factJson)
        : Cloudness(factJson["cloudness"].ForceNumber())
        , PrecStrength(factJson["prec_strength"].ForceNumber())
        , Temp(factJson["temp"].ForceNumber())
        , FeelsLike(factJson["feels_like"].ForceNumber())
        , PrecType(factJson["prec_type"].ForceIntNumber())
        , Icon(factJson["icon"])
        , Condition(factJson["condition"]) {
    }

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const {
        return MakeIconUrl(Icon, size, theme);
    }

    bool IsPrecipitation() const {
        return PrecStrength > 0.0;
    }
};

class TDay;

class TShortDayPart {
public:
    NAlice::TDateTime::EDayPart Type;

    double Temp;
    double FeelsLike;
    double Cloudness;
    double PrecStrength;

    TString Icon;
    TString Condition;

    TDay* Day;

public:
    TShortDayPart(const NSc::TValue& partJson, const NAlice::TDateTime::EDayPart type)
        : Type(type)
        , Temp(partJson["temp"].ForceNumber())
        , FeelsLike(partJson["feels_like"].ForceNumber())
        , Cloudness(partJson["cloudness"].ForceNumber())
        , PrecStrength(partJson["prec_strength"].ForceNumber())
        , Icon(partJson["icon"])
        , Condition(partJson["condition"]) {
    }

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const {
        return MakeIconUrl(Icon, size, theme);
    }

    TString BackgroundStyleClass() const {
        if (Type == NAlice::TDateTime::EDayPart::Morning || Type == NAlice::TDateTime::EDayPart::Day) {
            return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_day";
        }

        return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_night";
    }
};

class TDayPart {
public:
    NAlice::TDateTime::EDayPart Type;

    double TempMin;
    double TempMax;
    double TempAvg;
    double FeelsLike;
    double Cloudness;
    double PrecStrength;
    size_t PrecType;

    TString Icon;
    TString Condition;

    TDay* Day;

public:
    TDayPart(const NSc::TValue& partJson, const NAlice::TDateTime::EDayPart type)
        : Type(type)
        , TempMin(partJson["temp_min"].ForceNumber())
        , TempMax(partJson["temp_max"].ForceNumber())
        , TempAvg(partJson["temp_avg"].ForceNumber())
        , FeelsLike(partJson["feels_like"].ForceNumber())
        , Cloudness(partJson["cloudness"].ForceNumber())
        , PrecStrength(partJson["prec_strength"].ForceNumber())
        , PrecType(partJson["prec_type"].ForceIntNumber())
        , Icon(partJson["icon"])
        , Condition(partJson["condition"]) {
    }

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const {
        return MakeIconUrl(Icon, size, theme);
    }

    TString BackgroundStyleClass() const {
        if (Type == NAlice::TDateTime::EDayPart::Morning || Type == NAlice::TDateTime::EDayPart::Day) {
            return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_day";
        }

        return CloudinessPrecCssStyle(Cloudness, PrecStrength) + "_night";
    }

    bool IsPrecipitation() const {
        return PrecStrength > 0.0;
    }
};

class TParts {
public:
    TDayPart Night;
    TDayPart Morning;
    TDayPart Day;
    TDayPart Evening;

    TShortDayPart DayShort;
    TShortDayPart NightShort;

public:
    TParts(const NSc::TValue& json)
        : Night(json["night"], NAlice::TDateTime::EDayPart::Night)
        , Morning(json["morning"], NAlice::TDateTime::EDayPart::Morning)
        , Day(json["day"], NAlice::TDateTime::EDayPart::Day)
        , Evening(json["evening"], NAlice::TDateTime::EDayPart::Evening)
        , DayShort(json["day_short"], NAlice::TDateTime::EDayPart::Day)
        , NightShort(json["night_short"], NAlice::TDateTime::EDayPart::Night) {
    }
};

class THour {
public:
    size_t Hour;
    TInstant HourTS;
    double Temp;
    TString Icon;
    double PrecStrength;
    size_t PrecType;

    // TODO: This should be an iterator
    THour* Next = nullptr;
    TDayPart* DayPart = nullptr;

public:
    THour(const NSc::TValue& hourJson)
        : Hour(FromString<int>(hourJson["hour"]))
        , HourTS(TInstant::Seconds(hourJson["hour_ts"].GetIntNumber()))
        , Temp(hourJson["temp"].ForceNumber())
        , Icon(hourJson["icon"])
        , PrecStrength(hourJson["prec_strength"].ForceNumber())
        , PrecType(hourJson["prec_type"].ForceIntNumber()) {
    }

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const {
        return MakeIconUrl(Icon, size, theme);
    }

    bool IsPrecipitation() const {
        return PrecStrength > 0.0;
    }
};

class TDay {
public:
    NAlice::TDateTime::TSplitTime Date;

    TString Sunrise;
    TString Sunset;

    TParts Parts;
    TVector<THour> Hours;
    // TODO: This should be an iterator
    TDay* Next = nullptr;

public:
    TDay(const NSc::TValue& dayJson, const NDatetime::TTimeZone& tz)
        : Date(NAlice::TDateTime::TSplitTime(tz, dayJson["date_ts"].ForceIntNumber()))
        , Sunrise(dayJson["sunrise"])
        , Sunset(dayJson["sunset"])
        , Parts(dayJson["parts"]) {
        for (const auto& hour : dayJson["hours"].GetArray()) {
            Hours.push_back(THour(hour));
        }

        for (size_t i = 1; i < Hours.size(); ++i) {
            Hours[i - 1].Next = &Hours[i];
        }
    }
};

class TForecast {
public:
    size_t Now;
    TString NowDt;

    TInfo Info;
    TGeoObject GeoObject;
    TYesterday Yesterday;

    NSc::TValue L10n;

    TFact Fact;
    TVector<TDay> Days;

    NAlice::TDateTime::TSplitTime UserTime;

public:
    TForecast(const NSc::TValue& json)
        : Now(json["now"].ForceIntNumber())
        , NowDt(json["now_dt"])
        , Info(json["info"])
        , GeoObject(json["geo_object"])
        , Yesterday(json["yesterday"])
        , L10n(json["l10n"])
        , Fact(json["fact"])
        , UserTime(NAlice::TDateTime::TSplitTime(NDatetime::GetTimeZone(Info.TzInfo.TimeZoneName()), Now)) {
        const auto& userTimeZone = NDatetime::GetTimeZone(Info.TzInfo.TimeZoneName());
        for (const auto& dayJson : json["forecasts"].GetArray()) {
            Days.push_back(TDay(dayJson, userTimeZone));
        }

        for (size_t i = 1; i < Days.size(); ++i) {
            Days[i - 1].Next = &Days[i];
            if (!Days[i - 1].Hours.empty() && !Days[i].Hours.empty()) {
                Days[i - 1].Hours[Days[i - 1].Hours.size() - 1].Next = &Days[i].Hours[0];
            }
        }

        for (auto& day : Days) {
            day.Parts.Day.Day = &day;
            day.Parts.Night.Day = &day;
            day.Parts.Morning.Day = &day;
            day.Parts.Evening.Day = &day;
            day.Parts.DayShort.Day = &day;
            day.Parts.NightShort.Day = &day;

            for (auto& hour : day.Hours) {
                const NAlice::TDateTime::TSplitTime hourST {userTimeZone, 0, 0, 0, (ui32) hour.Hour};
                const auto dayPartEnum = NAlice::TDateTime::TimeToDayPart(hourST);

                switch (dayPartEnum) {
                    case NAlice::TDateTime::EDayPart::Night:
                        hour.DayPart = &day.Parts.Night;
                        break;
                    case NAlice::TDateTime::EDayPart::Morning:
                        hour.DayPart = &day.Parts.Morning;
                        break;
                    case NAlice::TDateTime::EDayPart::Day:
                        hour.DayPart = &day.Parts.Day;
                        break;
                    default:
                        hour.DayPart = &day.Parts.Evening;
                        break;
                }
            }
        }
    }

    const TDay& Today() const {
        return Days[0];
    }

    const TDay& Tomorrow() const {
        return Days[1];
    }

    const TMaybe<TDay> FindDay(const NAlice::TDateTime& dt) const {
        for (const auto& day : Days) {
            if (day.Date.ToString("%F") == dt.SplitTime().ToString("%F")) {
                return day;
            }
        }

        return TMaybe<TDay>();
    }

    const TMaybe<TDayPart> FindDayPart(const NAlice::TDateTime& dt) const {
        const auto& day = FindDay(dt);

        if (!day) {
            return TMaybe<TDayPart>();
        }

        switch (dt.DayPart()) {
            case NAlice::TDateTime::EDayPart::Morning:
                return day->Parts.Morning;
            case NAlice::TDateTime::EDayPart::Day:
                return day->Parts.Day;
            case NAlice::TDateTime::EDayPart::Evening:
                return day->Parts.Evening;
            default:
                return day->Parts.Night;
        }
    }

    const TMaybe<THour> FindHour(const NAlice::TDateTime& dt) const {
        const auto& day = FindDay(dt);

        if (day && dt.SplitTime().Hour() >= 0 && static_cast<size_t>(dt.SplitTime().Hour()) < day->Hours.size()) {
            return day->Hours[dt.SplitTime().Hour()];
        }

        return TMaybe<THour>();
    }
};

std::variant<TForecast, TError> FetchForecastFromWeatherApi(TContext& ctx);

class TNowcast {
public:
    TString State;
    TString Icon;
    TString Condition;
    size_t Cloudness;
    size_t PrecType;
    size_t PrecStrength;
    bool IsThunder;
    size_t Time;
    TString Text;
    int Code;

public:
    TNowcast(const NSc::TValue& json)
        : State(json["State"])
        , Icon(json["Icon"])
        , Condition(json["Condition"])
        , Cloudness(json["Cloudiness"].ForceIntNumber())
        , PrecType(json["PrecType"].ForceIntNumber())
        , PrecStrength(json["PrecStrength"].ForceIntNumber())
        , IsThunder(json["IsThunder"].GetBool())
        , Time(json["Time"].ForceIntNumber())
        , Text(json["Text"])
        , Code(json["Code"].ForceIntNumber()) {
    }
};

std::variant<TNowcast, TError> FetchNowcastFromWeatherApi(TContext& ctx);

} // namespace NBASS::NWeather
