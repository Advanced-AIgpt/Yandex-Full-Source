#pragma once

#include <alice/hollywood/library/scenarios/weather/context/weather_protos.h>
#include <alice/hollywood/library/scenarios/weather/fwd.h>
#include <alice/hollywood/library/scenarios/weather/util/translations.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

#include <alice/library/datetime/datetime.h>

#include <weather/app_host/forecast_postproc/lib/forecast_postproc.h>

#include <util/generic/noncopyable.h>

namespace NAlice::NHollywood::NWeather {

TString CloudinessPrecCssStyle(const double cloudiness, const double precStrength);
TString MakeIconUrl(const TString& icon, const size_t size = 128, const TString& theme = "light");

class TTzInfo : public NNonCopyable::TNonCopyable {
public:
    TString Name;
    int Offset;

public:
    TTzInfo(const TWeatherProtos::TPreparedForecast& forecast);

    TString TimeZoneName() const;
};

class TInfo : public NNonCopyable::TNonCopyable {
public:
    TTzInfo TzInfo;

public:
    TInfo(const TWeatherProtos::TPreparedForecast& forecast);
};

class TYesterday : public NNonCopyable::TNonCopyable {
public:
    double Temp;

public:
    TYesterday(const TWeatherProtos::TPreparedForecast& forecast);
};

class TFact : public NNonCopyable::TNonCopyable {
public:
    double Cloudness;
    double PrecStrength;
    double Temp;
    double FeelsLike;
    size_t PrecType;
    double WindSpeed;
    double WindGust;
    TString WindDir;
    int PressureMM;
    TString Icon;
    TString Condition;
    TString DayTime;
    TString Season;

public:
    TFact(const TWeatherProtos::TPreparedForecast& forecast);

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const;
    bool IsPrecipitation() const;
};

class TDay;

class TShortDayPart : public NNonCopyable::TNonCopyable {
public:
    NAlice::TDateTime::EDayPart Type;
    double Temp;
    double FeelsLike;
    double Cloudness;
    double PrecStrength;
    double WindSpeed;
    double WindGust;
    TString WindDir;
    int PressureMM;
    TString Icon;
    TString Condition;
    TDay* Day;
public:
    TShortDayPart(const TWeatherProtos::TDayPart& dayPart, const NAlice::TDateTime::EDayPart type);

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const;
    TString BackgroundStyleClass() const;
};

class TDayPart : public NNonCopyable::TNonCopyable {
public:
    NAlice::TDateTime::EDayPart Type;
    double TempMin;
    double TempMax;
    double TempAvg;
    double FeelsLike;
    double Cloudness;
    double PrecStrength;
    size_t PrecType;
    double WindSpeed;
    double WindGust;
    TString WindDir;
    int PressureMM;
    TString Icon;
    TString Condition;
    TString Source;
    TDay* Day;

public:
    TDayPart(const TWeatherProtos::TDayPart& dayPart, const NAlice::TDateTime::EDayPart type);

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const;
    TString BackgroundStyleClass() const;
    bool IsPrecipitation() const;
};

class TParts : public NNonCopyable::TNonCopyable {
public:
    TDayPart Night; // hours [0, 6]
    TDayPart Morning; // hours [6, 12]
    TDayPart Day; // hours [12, 18]
    TDayPart Evening; // hours [18, 24]

    TShortDayPart DayShort; // hours [6, 22]
    TShortDayPart NightShort; // hours [22, 6]

    THashMap<TStringBuf, TDayPart*> Map; // four day parts in a map

public:
    TParts(THashMap<TString, TWeatherProtos::TDayPart> dayPartsMap);
};

class THour : public NNonCopyable::TNonCopyable {
public:
    size_t Hour;
    TInstant HourTS;
    double Temp;
    TString Icon;
    double PrecStrength;
    size_t PrecType;
    double WindSpeed;
    double WindGust;
    TString WindDir;
    int PressureMM;
    // OLD TODO: This should be an iterator
    THour* Next = nullptr;
    TDayPart* DayPart = nullptr;

public:
    THour(const TWeatherProtos::TPreparedForecastHour& hour);

    TString IconUrl(const size_t size = 128, const TString& theme = "light") const;
    bool IsPrecipitation() const;
};

class TDay : public NNonCopyable::TNonCopyable {
public:
    NAlice::TDateTime::TSplitTime Date;
    TString Sunrise;
    TString Sunset;
    TParts Parts;
    TDeque<THour> Hours;
    // OLD TODO: This should be an iterator
    TDay* Next = nullptr;

public:
    TDay(const TWeatherProtos::TPreparedForecastItem& day, const NDatetime::TTimeZone& tz);
};

class TForecast : public NNonCopyable::TNonCopyable {
private:
    TWeatherProtos::TPreparedForecast PreparedForecast_;
public:
    size_t Now;
    TString NowDt;

    TInfo Info;
    TYesterday Yesterday;

    TTranslations L10n;

    TFact Fact;
    TDeque<TDay> Days;

    NAlice::TDateTime::TSplitTime UserTime;

    TVector<TWeatherProtos::TWarning> Warnings;
public:
    TForecast(const TWeatherContext& ctx);

    const TDay& Today() const;
    const TDay& Tomorrow() const;
    const TDay* FindDay(const NAlice::TDateTime& dt) const;
    const TDayPart* FindDayPart(const NAlice::TDateTime& dt) const;
    const THour* FindHour(const NAlice::TDateTime& dt) const;
};

class TNowcast : public NNonCopyable::TNonCopyable {
public:
    TString State;
    TString Icon;
    TString Condition;
    size_t Cloudness;
    size_t PrecType;
    size_t PrecStrength;
    size_t Time;
    TString Text;
    int Code;

public:
    TNowcast(const TWeatherContext& ctx);
};

} // namespace NAlice::NHollywood::NWeather
