#include "builtins.h"
#include "emoji.h"
#include "helpers.h"
#include "inflector.h"
#include "operators.h"

#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/string_utils/quote/quote.h>

#include <contrib/libs/double-conversion/double-conversion/double-conversion.h>
#include <contrib/libs/cctz/include/cctz/civil_time.h>
#include <contrib/libs/cctz/include/cctz/time_zone.h>
#include <library/cpp/timezone_conversion/convert.h>
#include <library/cpp/timezone_conversion/civil.h>
#include <contrib/libs/re2/re2/re2.h>

#include <util/charset/unidata.h>
#include <util/charset/utf8.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/random/random.h>
#include <util/stream/str.h>
#include <util/string/ascii.h>
#include <util/string/builder.h>
#include <util/string/escape.h>
#include <util/string/join.h>
#include <util/string/printf.h>
#include <util/string/split.h>
#include <util/string/strip.h>
#include <util/string/subst.h>
#include <util/string/vector.h>

#include <memory>

namespace NAlice::NNlg::NBuiltins {

namespace {

bool CheckSpace(const char c) {
    return IsAsciiSpace(c);
}

namespace NClientActionDirectiveKeys {

const TString Name{TStringBuf("name")};
const TString Payload{TStringBuf("payload")};
const TString Type{TStringBuf("type")};
const TString SubName{TStringBuf("sub_name")};

}  // namespace NClientActionDirectiveKeys

namespace NDateKeys {

const TString Year{TStringBuf("year")};
const TString Month{TStringBuf("month")};
const TString Day{TStringBuf("day")};
const TString Hour{TStringBuf("hour")};
const TString Minute{TStringBuf("minute")};
const TString Second{TStringBuf("second")};
const TString Microsecond{TStringBuf("microsecond")};
const TString Timezone{TStringBuf("tzinfo")};

}  // namespace NDateKeys

namespace NLangs {

const TString Ru{IsoNameByLanguage(::LANG_RUS)};
const TString Tr{IsoNameByLanguage(::LANG_TUR)};
const TString Ar{IsoNameByLanguage(::LANG_ARA)};

}

TStringBuf DEFAULT_LANG = NLangs::Ru;

TStringBuf ExtractLanguage(const TCallCtx& ctx) {
    return ctx.Language;
}

namespace NServerActionDirectiveKeys {

const TString Name{TStringBuf("name")};
const TString Payload{TStringBuf("payload")};
const TString Type{TStringBuf("type")};
const TString IgnoreAnswer{TStringBuf("ignore_answer")};

}  // namespace NServerActionDirectiveKeys

namespace NTime {

constexpr i64 YEAR_OFFSET = 1900;

}  // namespace NTime

class TMonths {
public:
    TMonths(
        const TStringBuf january,
        const TStringBuf february,
        const TStringBuf march,
        const TStringBuf april,
        const TStringBuf may,
        const TStringBuf june,
        const TStringBuf july,
        const TStringBuf august,
        const TStringBuf september,
        const TStringBuf october,
        const TStringBuf november,
        const TStringBuf december
        ) : Months_{january, february, march, april, may, june, july, august, september, october, november, december}
    {}

    const TStringBuf GetMonthName(int orderNumber) const {
        if (orderNumber >= 1 && orderNumber <= 12) {
            return this->Months_[orderNumber-1];
        } else {
            ythrow TValueError() << "orderNumber should be from 1 to 12, actual: " << orderNumber;
        }
    }

private:
    TVector<TStringBuf> Months_;
};

TStringBuf GetMonthName(const i64 month, const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TMonths> {
        {
            NLangs::Ru,
            TMonths{
                "январь",
                "февраль",
                "март",
                "апрель",
                "май",
                "июнь",
                "июль",
                "август",
                "сентябрь",
                "октябрь",
                "ноябрь",
                "декабрь"
            }
        },
        {
            NLangs::Ar,
            TMonths{
                "كانون الثاني", // January
                "شباط", // February
                "آذار", // March
                "نيسان", // April
                "أيار", // May
                "حزيران", // June
                "تموز", // July
                "آب", // August
                "أيلول", // September
                "تشرين الأول", // October
                "تشرين الثاني", // November
                "كانون الأول" // December
            }
        },
        {
            NLangs::Tr,
            TMonths{
                "Ocak", // January
                "Şubat", // February
                "Mart", // March
                "Nisan", // April
                "Mayıs", // May
                "Haziran", // June
                "Temmuz", // July
                "Ağustos", // August
                "Eylül", // September
                "Ekim", // October
                "Kasım", // November
                "Aralık" // December
            }
        },
    };
    if (const auto* result = LangMap.FindPtr(lang)) {
        return result->GetMonthName(month);
    }
    ythrow TValueError() << "unknown language, expected \"ru\" or \"ar\" or \"tr\", actual " << lang;
}

struct TWeekday {
    TWeekday(
        const TStringBuf monday,
        const TStringBuf tuesday,
        const TStringBuf wednesday,
        const TStringBuf thursday,
        const TStringBuf friday,
        const TStringBuf saturday,
        const TStringBuf sunday
    ) : Weekdays_{monday, tuesday, wednesday, thursday, friday, saturday, sunday}
    {}

    const TStringBuf GetWeekdayName(int orderNumber) const {
        if (orderNumber >= 1 && orderNumber <= 7) {
            return this->Weekdays_[orderNumber-1];
        } else {
            ythrow TValueError() << "orderNumber should be in [1, 7], actual: " << orderNumber;
        }
    }
private:
    TVector<TStringBuf> Weekdays_;
};

const THashMap<int, TStringBuf> WEEKDAY_GENDER = {
    {1, "m"},
    {2, "m"},
    {3, "f"},
    {4, "m"},
    {5, "f"},
    {6, "f"},
    {7, "n"},
};

const TWeekday& GetWeekdayByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TWeekday> {
        {
            NLangs::Ru,
            TWeekday(
                "понедельник",
                "вторник",
                "среда",
                "четверг",
                "пятница",
                "суббота",
                "воскресенье"
            )
        },
        {
            NLangs::Ar,
            TWeekday(
                "الاثنين", // Monday
                "الثلاثاء", // Tuesday
                "الأربعاء", // Wednesday
                "الخميس", // Thursday
                "الجمعة", // Friday
                "السبت", // Saturday
                "يوم الأحد" // Sunday
            )
        },
        {
            NLangs::Tr,
            TWeekday(
                "Pazartesi", // Monday
                "Salı", // Tuesday
                "Çarşamba", // Wednesday
                "Perşembe", // Thursday
                "Cuma", // Friday
                "Cumartesi", // Saturday
                "Pazar" // Sunday
            )
        },
    };
    if (const auto* result = LangMap.FindPtr(lang)) {
        return *result;
    }
    ythrow TValueError() << "unknown language, expected \"ru\" or \"ar\" or \"tr\", actual " << lang;
}

const TVector<int> WEEKDAYS = {1, 2, 3, 4, 5};

const TVector<int> WEEKENDS = {6, 7};

const TVector<int> ALL_WEEK = {1, 2, 3, 4, 5, 6, 7};

bool IsWeekday(const TVector<int>& days) {
    return days == WEEKDAYS;
}

bool IsWeekend(const TVector<int>& days) {
    return days == WEEKENDS;
}

bool IsAllWeek(const TVector<int>& days) {
    return days == ALL_WEEK;
}

struct TRelativeDay {
    const TStringBuf BeforeYesterday,
                     Yesterday,
                     Today,
                     Tomorrow,
                     AfterTomorrow;

    static inline TVector<int> SupportedRelative{-2, -1, 0, 1, 2};

    const TStringBuf GetRelativeDayName(int dayDiff) const {
        if (dayDiff == -2) {
            return this->BeforeYesterday;
        } else if (dayDiff == -1) {
            return this->Yesterday;
        } else if (dayDiff == 0) {
            return this->Today;
        } else if (dayDiff == 1) {
            return this->Tomorrow;
        } else if (dayDiff == 2) {
            return this->AfterTomorrow;
        } else {
            ythrow TValueError() << "dayDiff should be in [-2, 2], actual: " << dayDiff;
        }
    }
};

const TRelativeDay& GetRelativeDayByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TRelativeDay> {
        {
            NLangs::Ru,
            {
                .BeforeYesterday = "позавчера",
                .Yesterday = "вчера",
                .Today = "сегодня",
                .Tomorrow = "завтра",
                .AfterTomorrow = "послезавтра",
            }
        },
        {
            NLangs::Ar, {
                .BeforeYesterday = "قبل البارحة",
                .Yesterday = "البارحة",
                .Today = "اليوم",
                .Tomorrow = "الغد",
                .AfterTomorrow = "بعد غد",
            }
        },
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

const THashMap<TString, TString>& GetHumanTimeByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap< TStringBuf, THashMap<TString, TString>> {
        {
            NLangs::Ru, {
                {"0:00", "полночь"},
                {"0:30", "0:30"},
                {"1:00", "1 час ночи"},
                {"1:30", "1:30 ночи"},
                {"2:00", "2 ночи"},
                {"2:30", "2:30 ночи"},
                {"3:00", "3 ночи"},
                {"3:30", "3:30 ночи"},
                {"4:00", "4 утра"},
                {"4:30", "4:30 утра"},
                {"5:00", "5 утра"},
                {"5:30", "5:30 утра"},
                {"6:00", "6 утра"},
                {"6:30", "6:30 утра"},
                {"7:00", "7 утра"},
                {"7:30", "7:30 утра"},
                {"8:00", "8 утра"},
                {"8:30", "8:30 утра"},
                {"9:00", "9 утра"},
                {"9:30", "9:30 утра"},
                {"10:00", "10 утра"},
                {"10:30", "10:30 утра"},
                {"11:00", "11 утра"},
                {"11:30", "11:30 утра"},
                {"12:00", "12 часов дня"},
                {"12:30", "12:30 дня"},
                {"13:00", "1 час дня"},
                {"14:00", "2 часа дня"},
                {"15:00", "3 часа дня"},
                {"16:00", "4 часа дня"},
                {"17:00", "5 вечера"},
                {"18:00", "6 вечера"},
                {"19:00", "7 вечера"},
                {"20:00", "8 вечера"},
                {"21:00", "9 вечера"},
                {"22:00", "10 вечера"},
                {"23:00", "11 вечера"}
            }
        },
        {
            NLangs::Ar, {
                {"0:00", "منتصف الليل"},
                {"0:30", "0:30"},
                {"1:00", "الساعة 1 ليلاً"},
                {"1:30", "1:30 ليلاً"},
                {"2:00", "2 ليلاً"},
                {"2:30", "2:30 ليلاً"},
                {"3:00", "3 ليلاً"},
                {"3:30", "3:30 ليلاً"},
                {"4:00", "4 صباحاً"},
                {"4:30", "4:30 صباحاً"},
                {"5:00", "5 صباحاً"},
                {"5:30", "5:30 صباحاً"},
                {"6:00", "6 صباحاً"},
                {"6:30", "6:30 صباحاً"},
                {"7:00", "7 صباحاً"},
                {"7:30", "7:30 صباحاً"},
                {"8:00", "8 صباحاً"},
                {"8:30", "8:30 صباحاً"},
                {"9:00", "9 صباحاً"},
                {"9:30", "9:30 صباحاً"},
                {"10:00", "10 صباحاً"},
                {"10:30", "10:30 صباحاً"},
                {"11:00", "11 صباحاً"},
                {"11:30", "11:30 صباحاً"},
                {"12:00", "الساعة 12 ظهراً"},
                {"12:30", "12:30 ظهراً"},
                {"13:00", "الساعة 1 ظهراً"},
                {"14:00", "الساعة 2 ظهراً"},
                {"15:00", "الساعة 3 عصراً"},
                {"16:00", "الساعة 4 عصراً"},
                {"17:00", "5 مساءً"},
                {"18:00", "6 مساءً"},
                {"19:00", "7 مساءً"},
                {"20:00", "8 مساءً"},
                {"21:00", "9 مساءً"},
                {"22:00", "10 مساءً "},
                {"23:00", "11 مساءً"}
            }
        }
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

struct TDatetimeComponents {
    const TStringBuf Years;
    const TStringBuf Months;
    const TStringBuf Weeks;
    const TStringBuf Days;
    const TStringBuf Hours;
    const TStringBuf Minutes;
    const TStringBuf Seconds;

    const TStringBuf GetDateTimeComponent(const TStringBuf attribute) const {
        if (attribute.equal("years")) {
            return this->Years;
        } else if (attribute.equal("months")) {
            return this->Months;
        } else if (attribute.equal("weeks")) {
            return this->Weeks;
        } else if (attribute.equal("days")) {
            return this->Days;
        } else if (attribute.equal("hours")) {
            return this->Hours;
        } else if (attribute.equal("minutes")) {
            return this->Minutes;
        } else if (attribute.equal("seconds")) {
            return this->Seconds;
        } else {
            ythrow TValueError() << "unknown attribute:" << attribute;
        }
    }
};

const TDatetimeComponents& GetDatetimeRawComponentsByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TDatetimeComponents> {
        {
            NLangs::Ru,
            {
                .Years = "год",
                .Months = "месяц",
                .Weeks = "неделя",
                .Days = "день",
                .Hours = "час",
                .Minutes = "минута",
                .Seconds = "секунда",
            }
        },
        {
            NLangs::Ar,
            {
                .Years = "السنة",
                .Months = "الشهر",
                .Weeks = "الأسبوع",
                .Days = "اليوم",
                .Hours = "الساعة",
                .Minutes = "دقيقة",
                .Seconds = "ثانية",
            }
        }
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

const THashMap<TStringBuf, TStringBuf> CASES_FOR_SK = {
    {"nomn", "nom"},
    {"nom", "nom"},
    {"gent", "gen"},
    {"gen", "gen"},
    {"datv", "dat"},
    {"dat", "dat"},
    {"accs", "acc"},
    {"acc", "acc"},
    {"ablt", "instr"},
    {"instr", "instr"},
    {"ins", "instr"},
    {"loct", "loc"},
    {"abl", "loc"},
    {"loc", "loc"},
};

struct TDaysPeriod {
    const TStringBuf OnWeekDaysRepeated;
    const TStringBuf OnWeekDaySingleTime;
    const TStringBuf OnWeekendsRepeated;
    const TStringBuf OnWeekendSingleTime;
    const TStringBuf EveryDay;
};

const TDaysPeriod& GetDaysPeriodByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TDaysPeriod> {
        {
            NLangs::Ru,
            {
                .OnWeekDaysRepeated = "по будням",
                .OnWeekDaySingleTime = "в будни",
                .OnWeekendsRepeated = "по выходным",
                .OnWeekendSingleTime = "в выходные",
                .EveryDay = "каждый день",
            }
        },
        {
            NLangs::Ar,
            {
                .OnWeekDaysRepeated = "في أيام الأسبوع",
                .OnWeekDaySingleTime = "أيام الأسبوع",
                .OnWeekendsRepeated = "في أيام العطلة",
                .OnWeekendSingleTime = "أيام العطلة",
                .EveryDay = "كل يوم",
            }
        }
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

const THashMap<TStringBuf, TString>& GetUnitsTagsByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, THashMap<TStringBuf, TString>> {
        {
            NLangs::Ru, {
                {"#kilogram", "килограмм"},
                {"#meter", "метр"},
                {"#centimeter", "сантиметр"},
                {"#gram", "грамм"},
                {"#kilometer", "километр"},
                {"#minute", "минута"},
                {"#square_kilometer", "квадратный километр"},
            }
        },
        {
            NLangs::Ar, {
                {"#kilogram", "كيلوغرام"},
                {"#meter", "متر"},
                {"#centimeter", "سنتيمتر"},
                {"#gram", "جرام"},
                {"#kilometer", "كيلومتر"},
                {"#minute", "دقيقة"},
                {"#square_kilometer", "كيلومتر مربع"},
            }
        }
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

struct TPrepcase {
    const TString On_WeekDaysRepeated;
    const TString On_WeekDaySingleTime;
    const TString On_WeekDaySingleTime2;
    const TString In_MonthSingleTime;
    const TString In_YearSingleTime;
    const TString And;
    const TString InCity;
    const TString Prev_WeekDay;
    const TString Next_WeekDay;
    const TString Now;
    const TString OnThisWeek;
    const TString InThisMonth;
    const TString InThisYear;
    const TString After_TimeRelative;
    const TString Backward_TimeRelative;

    const TString At_TimeAbsolute;
    const TString For_Date;
};

const TPrepcase& GetPrepCaseByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TPrepcase> {
        {
            NLangs::Ru,
            {
                .On_WeekDaysRepeated = "по",
                .On_WeekDaySingleTime = "в",
                .On_WeekDaySingleTime2 = "во",
                .In_MonthSingleTime = "в",
                .In_YearSingleTime = "в",
                .And = "и",
                .InCity = "в городе",
                .Prev_WeekDay = "прошлый",
                .Next_WeekDay = "следующий",
                .Now = "сейчас",
                .OnThisWeek = "на этой неделе",
                .InThisMonth = "в этом месяце",
                .InThisYear = "в этом году",
                .After_TimeRelative = "через",
                .Backward_TimeRelative = "назад",
                .At_TimeAbsolute = "в",
                .For_Date = "на",
            }
        },
        {
            NLangs::Ar,
            {
                .On_WeekDaysRepeated = "مدة",
                .On_WeekDaySingleTime = "في",
                .On_WeekDaySingleTime2 = "في",
                .In_MonthSingleTime = "في",
                .In_YearSingleTime = "في",
                .And = "و",
                .InCity = "في مدينة",
                .Prev_WeekDay = "الماضية",
                .Next_WeekDay = "التالي",
                .Now = "الحين",
                .OnThisWeek = "في هذا الأسبوع",
                .InThisMonth = "في هذا الشهر",
                .InThisYear = "في هذا العام",
                .After_TimeRelative = "خلال",
                .Backward_TimeRelative = "الماضي",
                .At_TimeAbsolute = "يوم",
                .For_Date = "في",
            }
        }
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

struct TDayPartNameForTime {
    const TString OfNight;
    const TString OfMorning;
    const TString OfAfternoon;
};

const TDayPartNameForTime& GetDayPartNameForTimeByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TDayPartNameForTime> {
        {
            NLangs::Ru,
            {
                .OfNight = "ночи",
                .OfMorning = "утра",
                .OfAfternoon = "дня",
            }
        },
        {
            NLangs::Ar,
            {
                .OfNight = "ليلاً",
                .OfMorning = "صباحاً",
                .OfAfternoon = "ظهراً",
            }
        }
    };
    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}

struct TYearForms {
    const TString Year_Of;
    const TString Year_In;
};

const TYearForms& GetYearFormsByLang(const TStringBuf lang) {
    static const auto LangMap = THashMap<TStringBuf, TYearForms> {
        {
            NLangs::Ru,
            {
                .Year_Of = "года",
                .Year_In = "году",
            }
        },
        {
            NLangs::Ar,
            {
                .Year_Of = "السنة",
                .Year_In = "السنة",
            }
        }
    };

    static const auto& defaultResult = LangMap.at(DEFAULT_LANG);
    return LangMap.ValueRef(lang, defaultResult);
}


TString BuildOnNumberDayPhrase(const TStringBuf lang, int dayNumber) {
    static const auto langToStrTemplateMap = THashMap<TStringBuf, TStringBuf> {
        {NLangs::Ru, "на #acc %d-й день"},
        {NLangs::Ar, "في اليوم %d"},
    };
    if (const auto* strTemplate = langToStrTemplateMap.FindPtr(lang)) {
        return Sprintf(strTemplate->data(), dayNumber);
    } else {
        return ToString(dayNumber);
    }
}

const THashMap<TString, TString> DATETIME_REPLACEMENT_PREFIX = {
    {"через #acc 1 год", "через год"},
    {"через #acc 1 месяц", "через месяц"},
    {"через #acc 1 неделю", "через неделю"},
    {"через #acc 1 час", "через час"},
    {"#acc 1 год назад", "год назад"},
    {"#acc 1 месяц назад", "месяц назад"},
    {"#acc 1 неделю назад", "неделю назад"},
    {"#acc 1 час назад", "час назад"}
};

NDatetime::TTimeZone GetTimezone(const TValue& zoneName) {
    if (!zoneName.IsString() && !zoneName.IsNone()) {
        ythrow TTypeError() << "timezone must be a string, got a " << zoneName.GetTypeName();
    }

    if (zoneName.IsNone()) {
        return NDatetime::GetUtcTimeZone();
    }

    return NDatetime::GetTimeZone(zoneName.GetString().GetStr());
}

// TODO(a-square): consider making dates first-class objects
TMaybe<cctz::civil_day> ToDate(const TValue& year, const TValue& month, const TValue& day) {
    if (!year.IsInteger()) {
        return Nothing();
    }

    if (!month.IsInteger()) {
        return Nothing();
    }

    if (!day.IsInteger()) {
        return Nothing();
    }

    return cctz::civil_day(year.GetInteger(), month.GetInteger(), day.GetInteger());
}

// TODO(a-square): consider making dates first-class objects
TMaybe<cctz::civil_day> ToDate(const TValue& value) {
    return ToDate(GetAttrLoad(value, NDateKeys::Year),
                  GetAttrLoad(value, NDateKeys::Month),
                  GetAttrLoad(value, NDateKeys::Day));
}

TMaybe<NDatetime::TCivilSecond> ToDatetime(const TValue& year, const TValue& month, const TValue& day,
                                           const TValue& hour = TValue::Integer(0), const TValue& minute = TValue::Integer(0),
                                           const TValue& second = TValue::Integer(0),
                                           const TValue& microsecond = TValue::Integer(0),
                                           const TValue& timezone = TValue::None(),
                                           bool switchTimezone = true) {
    if (!year.IsInteger()) {
        return Nothing();
    }

    if (!month.IsInteger()) {
        return Nothing();
    }

    if (!day.IsInteger()) {
        return Nothing();
    }

    if (!hour.IsInteger()) {
        return Nothing();
    }

    if (!minute.IsInteger()) {
        return Nothing();
    }

    if (!second.IsInteger()) {
        return Nothing();
    }

    if (!microsecond.IsInteger()) {
        return Nothing();
    }

    if (!timezone.IsString() && !timezone.IsNone()) {
        return Nothing();
    }

    NDatetime::TCivilSecond civilSecond(year.GetInteger(), month.GetInteger(), day.GetInteger(),
                                        hour.GetInteger(), minute.GetInteger(), second.GetInteger());

    if (!timezone.IsNone() && switchTimezone) {
        return NDatetime::Convert<NDatetime::TCivilSecond>(civilSecond, GetTimezone(timezone), NDatetime::GetUtcTimeZone());
    } else {
        return civilSecond;
    }
}

TMaybe<NDatetime::TCivilSecond> ToDatetime(const TValue& value, bool switchTimezone = true) {
    return ToDatetime(GetAttrLoad(value, NDateKeys::Year),
                      GetAttrLoad(value, NDateKeys::Month),
                      GetAttrLoad(value, NDateKeys::Day),
                      GetAttrLoad(value, NDateKeys::Hour),
                      GetAttrLoad(value, NDateKeys::Minute),
                      GetAttrLoad(value, NDateKeys::Second),
                      GetAttrLoad(value, NDateKeys::Microsecond),
                      GetAttrLoad(value, NDateKeys::Timezone),
                      switchTimezone);
}

const std::vector<TString> DATETIME_FORMATS = {
    "%Y-%m-%d %H:%M:%S",
    "%Y-%m-%d %H:%M",
    "%Y-%m-%d %H",
    "%Y-%m-%d",
    "%Y %m %d %H:%M:%S",
    "%Y %m %d %H:%M",
    "%Y %m %d %H",
    "%Y %m %d",
    "%Y/%m/%d %H:%M:%S",
    "%Y/%m/%d %H:%M",
    "%Y/%m/%d %H",
    "%Y/%m/%d",
    "%Y.%m.%d %H:%M:%S",
    "%Y.%m.%d %H:%M",
    "%Y.%m.%d %H",
    "%Y.%m.%d",
    "%Y-%m-%d%ET%H:%M:%E*S%z",
    "%Y %m %d%ET%H:%M:%E*S%z",
    "%Y/%m/%d%ET%H:%M:%E*S%z",
    "%Y.%m.%d%ET%H:%M:%E*S%z",
};

std::chrono::system_clock::time_point ParseDatetime(const TValue& datetimeString) {
    const auto datetime = datetimeString.GetString().GetStr();
    for (const auto& format : DATETIME_FORMATS) {
        std::chrono::system_clock::time_point absTime;
        const cctz::time_zone tz = cctz::utc_time_zone();
        if (cctz::parse(format, datetime, tz, &absTime)) {
            return absTime;
        }
    }

    ythrow TValueError() << "undefined datetime_string format: " << datetime;
}

TValue ConvertTimezone(const TValue& time, const TValue& newTimezone) {
    TMaybe<NDatetime::TCivilSecond> civilTime = ToDatetime(time);

    if (!civilTime) {
        // Not a valid time, just return a source
        return time;
    }

    NDatetime::TCivilSecond newCivilTime = NDatetime::Convert<NDatetime::TCivilSecond>(*civilTime, NDatetime::GetUtcTimeZone(), GetTimezone(newTimezone));
    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(newCivilTime.year())},
        {NDateKeys::Month, TValue::Integer(newCivilTime.month())},
        {NDateKeys::Day, TValue::Integer(newCivilTime.day())},
        {NDateKeys::Hour, TValue::Integer(newCivilTime.hour())},
        {NDateKeys::Minute, TValue::Integer(newCivilTime.minute())},
        {NDateKeys::Second, TValue::Integer(newCivilTime.second())},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, newTimezone},
    });
}

const IInflector& GetInflector(const bool fio = false) {
    if (fio) {
        static const auto inflector = CreateFioInflector();
        return *inflector;
    } else {
        static const auto inflector = CreateNormalInflector();
        return *inflector;
    }
}

template <typename TGoodCasePred, typename TBadCasePred>
TValue TestCharCase(const TValue& target, TGoodCasePred&& goodCasePred, TBadCasePred&& badCasePred,
                    const TStringBuf name) {
    // NOTE(a-square): the Python original casts to str, we don't want that
    if (!target.IsString()) {
        ythrow TTypeError() << name << "test expects a string, got a " << target.GetTypeName();
    }

    const TStringBuf str = target.GetString().GetBounds();

    bool gotCase = false; // for compatibility with Python's str.islower()/str.isupper()
    RECODE_RESULT retcode = RECODE_OK;
    const unsigned char* current = reinterpret_cast<const unsigned char*>(str.data());
    const unsigned char* end = current + str.size();
    wchar32 rune = BROKEN_RUNE;
    while (current < end && RECODE_OK == (retcode = ReadUTF8CharAndAdvance(rune, current, end))) {
        if (goodCasePred(rune)) {
            gotCase = true;
        } else if (badCasePred(rune)) {
            return TValue::Bool(false);
        }
    }

    if (retcode != RECODE_OK) {
        ythrow TValueError() << "Malformed UTF-8 string";
    }

    return TValue::Bool(gotCase);
}

class TUtf8CharsStripAdapter {
public:
    explicit TUtf8CharsStripAdapter(const TStringBuf chars, const TStringBuf targetBuf)
        : Chars(TUtf32String::FromUtf8(chars))
        , TargetBuf(targetBuf) {
    }

    bool operator()(const char* it) const {
        // The adapter should only ever be called on characters inside the TargetBuf,
        // here we check this. This is an internal helper class so an assert is sufficient.
        Y_ASSERT(!std::less<void>{}(it, TargetBuf.begin()) && std::less<void>{}(it, TargetBuf.end()));
        const unsigned char* charBegin = reinterpret_cast<const unsigned char*>(it);
        const unsigned char* targetEnd = reinterpret_cast<const unsigned char*>(TargetBuf.end());

        RECODE_RESULT retcode = RECODE_ERROR;
        wchar32 rune = BROKEN_RUNE;
        size_t runeSize = 0;
        retcode = SafeReadUTF8Char(rune, runeSize, charBegin, targetEnd);
        Y_ASSERT(retcode == RECODE_OK);

        return Chars.Contains(rune);
    }

private:
    TUtf32String Chars;
    const TStringBuf TargetBuf;
};

TString PadString(const TStringBuf str, const TStringBuf padding) {
    TStringBuilder builder;
    builder << padding;

    size_t offset = 0;
    while (offset < str.size()) {
        const size_t runeLen = UTF8RuneLen(str[offset]);
        Y_ENSURE(runeLen > 0); // not an assert because we'd *really* hate an infinite loop here!

        Y_ASSERT(offset + runeLen <= str.size());  // UTF-8 assumption checked at the system boundary
        builder << TStringBuf{str.begin() + offset, runeLen};
        builder << padding;

        offset += runeLen;
    }

    return builder;
}

TString Div2EscapeImpl(const TStringBuf str) {
    TString result(Reserve(str.size()));

    for (const char c : str) {
        if (c == '\"' || c == '\\' || c == '\r' || c == '\n' || c == '\t') {
            result += EscapeC(c);
        } else if (static_cast<const unsigned char>(c) >= 0x20) {
            // We skip <0x20 control symbols to be compatible with json format
            // https://a.yandex-team.ru/arc_vcs/contrib/libs/rapidjson/include/rapidjson/reader.h?rev=r9037227#L959
            result.push_back(c);
        }
    }

    return result;
}

// XXX(a-square): we could use util's EncodeHtmlPcdata if we didn't have to encode the newline as well
TString HtmlEscapeImpl(const TStringBuf str) {
    TString result(Reserve(str.size()));

    for (const char c : str) {
        TStringBuf escapedChar;
        switch (c) {
        case '\n':
            escapedChar = TStringBuf("<br/>");
            break;
        case '&':
            escapedChar = TStringBuf("&amp;");
            break;
        case '"':
            escapedChar = TStringBuf("&quot;");
            break;
        case '\'':
            escapedChar = TStringBuf("&apos;");
            break;
        case '<':
            escapedChar = TStringBuf("&lt;");
            break;
        case '>':
            escapedChar = TStringBuf("&gt;");
            break;
        case '\\':
            escapedChar = TStringBuf("&#92;");
            break;
        default:
            result.push_back(c);
            continue;
        }

        Y_ASSERT(escapedChar);
        result += escapedChar;
    }

    return result;
}

// performs exact double rounding the way Python does it (but with different Grisu3 implementation),
// https://github.com/python/cpython/blob/2.7/Objects/floatobject.c#L1091
// - uses older dtoa primitive, we want the spirit of this function but not its clunky implemenation
// https://github.com/python/cpython/blob/3.8/Objects/floatobject.c#L936
// - uses better dtoa so the entire function is nicer, we use the same algo (courtesy of Google)
double ExactRound(double x, int numDigits) {
    using TConverter = double_conversion::DoubleToStringConverter;

    if (numDigits == 0) {
        double rounded = std::round(x);

        // Halfway between two integers; use round-away-from-zero.
        // Python uses the reduced precision fabs() function, and so do we
        if (std::fabs(rounded - x) == 0.5) {
            rounded = x + (x > 0.0 ? 0.5 : -0.5);
        }
    }

    // NOTE(a-square): double_conversion's DoubleToAscii doesn't understand negative precision rounding,
    // so we fake it, it shouldn't lead to overflow and it's not an important case for us in practice
    int borrowedDigits = 0;
    if (numDigits < 0) {
        x /= std::pow(10.0, -numDigits);
        borrowedDigits = -numDigits;
        numDigits = 0;
    }

    Y_ASSERT(borrowedDigits >= 0);

    // DoubleToStringConverter::kBase10MaximalLength = 17, not marked constexpr,
    // at most 8 more characters are needed to tweak the number in case no digits were borrowed.
    // We might have up to 20 borrowed digits.
    //
    // We jack the size way up high to avoid any chance of overflow.
    //
    // For some reason Python does another dynamic check to ensure everything fits,
    // we don't need that because of the guarantee above
    constexpr size_t dtoaBufferSize = 100;
    char dtoaBuffer[dtoaBufferSize];

    // perform rounding and store the digits (w/0 sign or decimal point)
    bool negative;  // the sign bit
    int length;  // length of the resulting string
    int point;  // the position where we would've put the decimal point
    TConverter::DoubleToAscii(x, TConverter::FIXED, numDigits,
                              dtoaBuffer + 2, dtoaBufferSize - 2,  // leave 2 bytes in front, see below
                              &negative, &length, &point);

    // prefix the number with its sign and a leading zero the way Python does it
    dtoaBuffer[0] = negative ? '-' : '+';
    dtoaBuffer[1] = '0';

    Y_ASSERT(length >= 0);
    size_t endOffset = static_cast<size_t>(length) + 2;
    Y_ENSURE(endOffset < dtoaBufferSize);

    if (borrowedDigits > 0) {
        for (int i = 0; i < borrowedDigits; ++i) {
            dtoaBuffer[endOffset++] = '0';
            Y_ENSURE(endOffset < dtoaBufferSize);
        }
    }

    // write the exponent
    dtoaBuffer[endOffset] = 'e';
    ++endOffset;
    Y_ENSURE(endOffset < dtoaBufferSize);

    const int charsWritten = snprintf(dtoaBuffer + endOffset, dtoaBufferSize - endOffset, "%d", point - length);
    Y_ASSERT(charsWritten > 0);
    endOffset += static_cast<size_t>(charsWritten);
    Y_ENSURE(endOffset < dtoaBufferSize);

    return FromString<double>(TStringBuf{dtoaBuffer, endOffset});
}

bool IsUnreservedChar(unsigned char c) {
    // TODO(a-square): clang puts 4 conditional jumps here, consider hand-rolling a LUT
    return IsAsciiAlnum(c) || c == '-' || c == '.' || c == '_' || c == '~';
}

// copied from util/string/quote.cpp
unsigned char DigitToHex(unsigned char x) {
    Y_ASSERT(x <= 16);
    return static_cast<unsigned char>((x < 10) ? ('0' + x) : ('A' + x - 10));
}

// computes the length of UrlEncodeImpl's result
size_t GetEncodedSize(const TStringBuf in) {
    size_t result = 0;
    for (unsigned char c : in) {
        if (IsUnreservedChar(c)) {
            ++result;
        } else {
            result += 3;
        }
    }
    return result;
}

// We use our own encoding rules because util's UrlEscape doesn't quote all necessary symbols.
// Per https://tools.ietf.org/html/rfc3986#section-2.3 we need to encode all symbols that
// aren't specifically called unreserved to be safe
TString UrlEncodeImpl(const TStringBuf in) {
    const size_t outSize = GetEncodedSize(in);
    auto out = TString::Uninitialized(outSize);
    const auto begin = out.begin();
    auto ptr = begin;

    for (const unsigned char c : in) {
        if (IsUnreservedChar(c)) {
            Y_ASSERT(static_cast<size_t>(std::distance(begin, ptr)) < outSize);
            *ptr++ = c;
            continue;
        }

        Y_ASSERT(static_cast<size_t>(std::distance(begin, ptr)) + 2 < outSize);
        *ptr++ = '%';
        *ptr++ = DigitToHex(c >> 4);
        *ptr++ = DigitToHex(c & 0xF);
    }

    Y_ASSERT(static_cast<size_t>(std::distance(begin, ptr)) == outSize);

    return out;
}

bool TestSequenceImpl(const TValue& target) {
    // TODO(a-square): consider making string a sequence
    return target.IsList() || target.IsRange();
}

TValue GetItemImpl(const TValue& target, const TStringBuf key, const TValue& defValue) {
    TStringBuf keyFirst = key;
    TStringBuf keyRest;
    key.TrySplit('.', keyFirst, keyRest);

    i64 index = ::Min<i64>();
    TValue item = TValue::Undefined();
    if (target.IsDict()) {
        item = GetAttrLoad(target, keyFirst);
    } else if (TestSequenceImpl(target) && TryFromString<i64>(keyFirst, index)) {
        item = GetItemLoadInt(target, index);
    }

    if (item.IsUndefined()) {
        return defValue;
    }

    if (keyRest) {
        return GetItemImpl(item, keyRest, defValue);
    }

    return item;
}

TString StripParenPairs(const TStringBuf str) {
    auto curr = str.begin();
    const auto end = str.end();

    TString outStr;
    TStringOutput out{outStr};

    while (curr != end) {
        // invariant:
        // - [begin, curr) has been processed
        // - curr <= openParen <= closeParen <= end
        auto openParen = std::find(curr, end, '(');
        const auto closeParen = std::find(openParen, end, ')');

        if (closeParen == end) {
            // no closing parenthesis found, finish without trimming
            openParen = end;
        }

        // we will always make forward progress because the only way we aren't going to advance curr
        // is if curr == end, and that's our stopping condition
        out << TStringBuf{curr, openParen};

        // avoid overshooting by ending the loop if no closing parenthesis was found
        if (closeParen == end) {
            break;
        }
        curr = closeParen + 1;
    }

    return StripString(outStr);
}

template<typename ChangeFunc>
TValue ChangeFirst(const TValue& target, ChangeFunc changeFunc) {
    const TText& str = target.GetString();
    const TStringBuf bounds = str.GetBounds();

    // strip the string from the left
    const TStringBuf strBuf = StripStringLeft(bounds);  // assume valid UTF-8
    if (strBuf.empty()) {
        return target;
    }
    const size_t firstCharOffset = strBuf.data() - bounds.data();

    // decode the first char's Unicode code point
    wchar32 firstCharRune = BROKEN_RUNE;
    size_t firstCharSize = 0;
    RECODE_RESULT retcode = SafeReadUTF8Char(firstCharRune, firstCharSize,
                                             reinterpret_cast<const unsigned char*>(strBuf.begin()),
                                             reinterpret_cast<const unsigned char*>(strBuf.end()));
    Y_ENSURE(retcode == RECODE_OK);

    // upcase the first char
    const wchar32 upperCharRune = changeFunc(firstCharRune);
    Y_ENSURE(upperCharRune != BROKEN_RUNE);

    // convert the first char's uppercased code point back to UTF-8
    char firstCharUpperBuf[5];
    size_t firstCharUpperSize = 0;
    retcode = SafeWriteUTF8Char(upperCharRune, firstCharUpperSize,
                                reinterpret_cast<unsigned char*>(firstCharUpperBuf),
                                sizeof(firstCharUpperBuf));
    Y_ENSURE(retcode == RECODE_OK);

    // reconstruct the string, respect the target's flags
    TText result;
    result.Append({firstCharUpperBuf, firstCharUpperSize}, str.GetFlagsAt(firstCharOffset));
    result.Append(str.GetView(str.GetBounds().substr(firstCharOffset + firstCharSize)));
    return TValue::String(std::move(result));
}

NDatetime::TSimpleTM GetNowCivilTime(const TValue& timezone, const TMaybe<TInstant> mockedTime = Nothing()) {
    TInstant now = mockedTime.Defined() ? *mockedTime : TInstant::Now();

    if (timezone.IsNone()) {
        return NDatetime::ToCivilTime(now, NDatetime::GetLocalTimeZone());
    } else if (timezone.IsString()) {
        NDatetime::TTimeZone tz = NDatetime::GetTimeZone(timezone.GetString().GetStr());
        return NDatetime::ToCivilTime(now, tz);
    } else {
        ythrow TTypeError() << "timezone must be a string or none, got a " << timezone.GetTypeName();
    }
}

TString ConstructHumanTime(const TCallCtx& ctx, const TGlobalsChain*, const TValue& time) {
    const TValue dt = ConvertTimezone(time, TValue::None()); // utc timezone

    int hour = GetAttrLoad(dt, NDateKeys::Hour).GetInteger();
    int minute = GetAttrLoad(dt, NDateKeys::Minute).GetInteger();

    const TStringBuf lang = ExtractLanguage(ctx);
    const THashMap<TString, TString>& mapping = GetHumanTimeByLang(lang);

    TStringBuilder str;
    str << hour << ":";
    if (minute < 10) {
        str << "0";
    }
    str << minute;

    if (const auto* res = mapping.FindPtr(str); res) {
        return *res;
    }
    return str;
}

TString ConstructHumanDate(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& date, bool withYear = false) {
    if (!date.IsDict()) {
        ythrow TTypeError() << "date must be a dict, got a " << date.GetTypeName();
    }

    const TValue humanMonth = HumanMonth(ctx, globals, date, TValue::String("gen"));

    TSmallVec<TString> res;
    res.push_back(ToString(GetAttrLoad(date, NDateKeys::Day).GetInteger()));
    res.push_back(humanMonth.GetString().GetStr());

    if (withYear) {
        res.push_back(ToString(GetAttrLoad(date, NDateKeys::Year).GetInteger()));
        const TStringBuf lang = ExtractLanguage(ctx);
        res.push_back(GetYearFormsByLang(lang).Year_Of);
    }

    return JoinStrings(res.begin(), res.end(), " ");
}

} // namespace

TValue Abs(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (const auto maybeInt = ToInteger(target)) {
        if (*maybeInt == ::Min<i64>()) {
            ythrow TValueError() << "abs got " << *maybeInt
                                 << " as its target, we cannot properly represent its absolute value";
        }
        return TValue::Integer(::Abs(*maybeInt));
    }

    if (const auto maybeDouble = ToDouble(target)) {
        return TValue::Double(::Abs(*maybeDouble));
    }

    ythrow TTypeError() << "abs expects a numeric value as its target, got " << target.GetTypeName();
}

TValue Attr(const TCallCtx&, const TGlobalsChain*, const TValue& value, const TValue& name) {
    // XXX(a-square): this thing behaves just like the dot operator
    // due to us not replicating anything close to Python's object model
    if (!name.IsString()) {
        ythrow TTypeError() << "attr filter expects a string as the attribute name, got " << name;
    }

    return GetAttrLoad(value, name.GetString().GetStr());
}

TValue Random(const TCallCtx& ctx, const TGlobalsChain*, const TValue& value) {
    struct {
        TValue operator()(TValue::TUndefined) const {
            ythrow TTypeError() << "random doesn't support undefined type";
        }

        TValue operator()(bool) const {
            ythrow TTypeError() << "random doesn't support boolean type";
        }

        TValue operator()(i64) const {
            ythrow TTypeError() << "random doesn't support integer type";
        }

        TValue operator()(double) const {
            ythrow TTypeError() << "random doesn't support float type";
        }

        TValue operator()(const TText& value) const {
            const auto& str = value.GetStr();
            auto index = Rng.RandomInteger(str.size());
            return TValue::String(str[index]);
        }

        TValue operator()(const TValue::TListPtr& listPtr) {
            auto& list = listPtr.Get();
            auto index = Rng.RandomInteger(list.size());
            return list[index];
        }

        TValue operator()(const TValue::TDictPtr&) {
            ythrow TTypeError() << "random doesn't support dict type";
        }

        TValue operator()(const TValue::TRangePtr& rangePtr) {
            auto& range = rangePtr.Get();
            auto index = Rng.RandomInteger(range.GetSize());
            return TValue::Integer(*range[index]);
        }

        TValue operator()(TValue::TNone) {
            ythrow TTypeError() << "random doesn't support None";
        }

        IRng& Rng;
    } visitor{ctx.Rng};
    return std::visit(visitor, value.GetData());
}

TValue Float(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& defValue) {
    const auto maybeDefDouble = ToDouble(defValue);
    if (!maybeDefDouble) {
        ythrow TTypeError() << "float expects a double as its default value, got " << defValue.GetTypeName();
    }
    double value = *maybeDefDouble;

    if (const auto maybeDouble = ToDouble(target)) {
        return TValue::Double(*maybeDouble);
    }

    if (target.IsString()) {
        TryFromString(target.GetString().GetBounds(), value);
    }

    return TValue::Double(value);
}

// this is a straight port of get_item from alice/vins/core/vins_core/nlg/filters.py
TValue GetItem(const TCallCtx&, const TGlobalsChain*,
               const TValue& target, const TValue& key, const TValue& defValue) {
    if (!TruthValue(target)) {
        return defValue;
    }

    if (key.IsInteger()) {
        return GetItemLoadInt(target, key.GetInteger());
    }

    if (!key.IsString()) {
        ythrow TTypeError() << "get_item expects a string or an integer as the key, got " << key.GetTypeName();
    }

    return GetItemImpl(target, key.GetString().GetBounds(), defValue);
}

TValue Round(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& precision, const TValue& method) {
    const auto maybeDouble = ToDouble(target);
    if (!maybeDouble) {
        ythrow TTypeError() << "round expects a value convertible to double as its target, got " << target.GetTypeName();
    }

    if (!precision.IsInteger()) {
        ythrow TTypeError() << "round expects an integer as its precision, got " << precision.GetTypeName();
    }

    const double targetDouble = *maybeDouble;
    const i64 precisionInt = precision.GetInteger();

    if (std::abs(precisionInt) > 20) {
        ythrow TValueError() << "round only supports up to 20 digits of rounding precision";
    }

    const auto checkMethod = [&method](const TStringBuf str) {
        return method.IsString() && method.GetString().GetBounds() == str;
    };

    if (method.IsNone() || checkMethod(TStringBuf("common"))) {
        return TValue::Double(ExactRound(targetDouble, static_cast<int>(precisionInt)));
    }

    const double exp = std::pow(10, precisionInt);

    // floor and ceil versions use inexact rounding that potentially loses precision;
    // this is how Jinja2 does it:
    // https://a.yandex-team.ru/arc/trunk/arcadia/contrib/python/Jinja2/jinja2/filters.py?rev=5438073&blame=true#L768
    if (checkMethod(TStringBuf("floor"))) {
        return TValue::Double(std::floor(targetDouble * exp) / exp);
    }

    if (checkMethod(TStringBuf("ceil"))) {
        return TValue::Double(std::ceil(targetDouble * exp) / exp);
    }

    ythrow TValueError() << "round expects its rounding method to be either common, floor, or ceil";
}

//
// @int
// Usage: {{ 123.4 | int }}
//
// Преобраование вещественых и строковых значений в int
//
TValue Int(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& defValue) {
    const auto maybeDefInt = ToInteger(defValue);
    if (!maybeDefInt) {
        ythrow TTypeError() << "int expects an integer as its default value, got " << defValue.GetTypeName();
    }
    i64 value = *maybeDefInt;

    if (const auto maybeInt = ToInteger(target)) {
        return TValue::Integer(*maybeInt);
    }

    if (target.IsDouble()) {
        return TValue::Integer(target.GetDouble());
    }

    if (target.IsString()) {
        // TODO(a-square): support base selection via another filter param if necessary
        TryFromString(target.GetString().GetBounds(), value);
    }

    return TValue::Integer(value);
}

//
// @inflect
// Usage: {{ string | inflect('gent', fio=True) }}
//
// Выбирает склонение для слов (только русский язык!)
// Need details!!!
//
TValue Inflect(const TCallCtx& ctx, const TGlobalsChain*, const TValue& target, const TValue& cases, const TValue& fio) {
    if (!target.IsString()) {
        ythrow TTypeError() << "inflect expects a string as its target, got " << target.GetTypeName();
    }

    const TStringBuf lang = ExtractLanguage(ctx);
    if (lang != NLangs::Ru) {
        return target;
    }

    if (!cases.IsString()) {
        ythrow TTypeError() << "inflect expects a string as its cases, got " << cases.GetTypeName();
    }

    const IInflector& inflector = GetInflector(/* fio = */ TruthValue(fio));
    return TValue::String(inflector.Inflect(target.GetString().GetStr(), cases.GetString().GetStr()));
}

//
// @default
// Usage: ???
//
// Выбирает значение по умолчанию, если переданный параметр не является корректным
//
TValue Default(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& defValue, const TValue& boolean) {
    const bool booleanFlag = TruthValue(boolean);
    if (booleanFlag && !TruthValue(target) || target.IsUndefined()) {
        return defValue;
    }

    return target;
}

//
// @div2_escape
// Usage: {{ skill | dev2_escape }}
//
// Выполняет экранирование символов " \ \r \n \t
//
TValue Div2Escape(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "escape expects a string as its target, got " << target.GetTypeName();
    }

    TText result;
    for (const auto [str, flags] : target.GetString()) {
        result.Append(Div2EscapeImpl(str), flags);
    }
    return TValue::String(std::move(result));
}

//
// @html_escape
// Usage: {{ skill | html_escape }}
//
// skill - объект типа String
// Выполняет преобразование символов в формате HTML
//
TValue HtmlEscape(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "html_escape expects a string as its target, got " << target.GetTypeName();
    }

    TText result;
    for (const auto [str, flags] : target.GetString()) {
        result.Append(HtmlEscapeImpl(str), flags);
    }
    return TValue::String(std::move(result));
}

//
// @format_weekday
// Usage:
// {{ date | format_weekday }}
//
// date - объект типа create_date_safe()
// Возвращает название дня недели в виде "понедельник", "вторник", ...
//
TValue FormatWeekday(const TCallCtx& ctx, const TGlobalsChain*, const TValue& target) {
    const auto maybeDate = ToDate(target);
    if (!maybeDate) {
        ythrow TTypeError() << "format_weekday expects a date as its target, got " << target.GetTypeName();
    }

    int weekday = ToUnderlying(cctz::get_weekday(*maybeDate));
    const TStringBuf lang = ExtractLanguage(ctx);
    return TValue::String(GetWeekdayByLang(lang).GetWeekdayName(weekday+1));
}

//
// @human_month
// Usage:
// {{ date | human_month('gen') }}
//
// date - объект типа create_date_safe()
// Возвращает название месяца с учетом падежа
//
TValue HumanMonth(const TCallCtx& ctx, const TGlobalsChain* globals,
                  const TValue& target, const TValue& grams) {

    const auto maybeDate = ToDate(target);
    if (!maybeDate) {
        ythrow TTypeError() << "human_month expects a date as its target, got " << target.GetTypeName();
    }

    const TStringBuf lang = ExtractLanguage(ctx);
    const auto result = TValue::String(GetMonthName(maybeDate->month(), lang));

    if (grams.IsNone()) {
        return result;
    }

    if (grams.IsString()) {
        return Inflect(ctx, globals, result, grams, /* fio = */ TValue::Bool(false));
    }

    ythrow TTypeError() << "human_month expects a string or none as grams, got " << grams.GetTypeName();
}

//
// @capitalize_first
// Usage:
// {{ name | capitalize_first }}
//
// name - объект типа строка
// Делает первую букву предложения заглавной
//
TValue CapitalizeFirst(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "capitalize_first expects a string as its target, got " << target.GetTypeName();
    }

    return ChangeFirst(target, [](wchar32 value) { return ToUpper(value); });
}

//
// @decapitalize_first
// Usage:
// {{ name | decapitalize_first }}
//
// name - объект типа строка
// Делает первую букву предложения строчной
//
TValue DecapitalizeFirst(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "decapitalize_first expects a string as its target, got " << target.GetTypeName();
    }

    return ChangeFirst(target, [](wchar32 value) { return ToLower(value); });
}

TValue ToJson(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    TString result;
    TStringOutput out(result);

    out << target.AsJson();
    return TValue::String(std::move(result));
}

TValue Trim(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    return StrStrip(ctx, globals, target, TValue::None());
}

TValue TtsDomain(const TCallCtx&, const TGlobalsChain*,
                 const TValue& target, const TValue& domain) {
    if (!domain.IsString()) {
        ythrow TTypeError() << "tts_domain expects a string as the domain, got " << domain.GetTypeName();
    }

    TText result;
    TTextOutput out(result);
    out << ClearFlag(TText::EFlag::Text)
        << TStringBuf("<[domain ")
        << domain.GetString().GetBounds()
        << TStringBuf("]>")
        << SetFlag(TText::EFlag::Text)
        << target
        << ClearFlag(TText::EFlag::Text)
        << TStringBuf("<[/domain]>");
    return TValue::String(result);
}

TValue Capitalize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    return CapitalizeFirst(ctx, globals, target);
}

TValue Decapitalize(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    return DecapitalizeFirst(ctx, globals, target);
}

TValue Urlencode(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    // TODO(a-square): support dict targets if needed
    if (!target.IsString()) {
        ythrow TTypeError() << "urlencode expects a string as its target, got " << target.GetTypeName();
    }

    // XXX(a-square): we reset the flags here, maybe it's wrong
    return TValue::String(UrlEncodeImpl(target.GetString().GetBounds()));
}

TValue Lower(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    return StrLower(ctx, globals, target);
}

TValue Map(const TCallCtx&, const TGlobalsChain*,
           const TValue& target, const TValue& filter, const TValue& attribute) {
    if (!TestSequenceImpl(target)) {
        ythrow TTypeError() << "map expects a sequence as its target, got " << target.GetTypeName();
    }

    if (!filter.IsNone()) {
        ythrow TValueError() << "map currently only supports attribute-based mapping";
    }

    if (!attribute.IsString()) {
        ythrow TTypeError() << "map expects a string as the attribute, got " << attribute.GetTypeName();
    }

    const TStringBuf attrBuf = attribute.GetString().GetBounds();

    TValue::TList result(Reserve(GetLength(target)));
    for (const auto& item : target) {
        result.push_back(GetAttrLoad(item, attrBuf));
    }
    return TValue::List(std::move(result));
}

TValue Max(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsList() || GetLength(target) == 0) {
        ythrow TTypeError() << "max expects a non-empty list as its target, got " << target.GetTypeName();
    }
    auto cmp = [](const TValue& lhs, const TValue& rhs) {
        return NOperators::Less(lhs, rhs).GetBool();
    };
    return *std::max_element(target.begin(), target.end(), cmp);
}

TValue Min(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsList() || GetLength(target) == 0) {
        ythrow TTypeError() << "min expects a non-empty list as its target, got " << target.GetTypeName();
    }
    auto cmp = [](const TValue& lhs, const TValue& rhs) {
        return NOperators::Less(lhs, rhs).GetBool();
    };
    return *std::min_element(target.begin(), target.end(), cmp);
}

TValue NumberOfReadableTokens(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (target.IsUndefined()) {
        return TValue::Integer(0);
    }

    if (!target.IsString()) {
        ythrow TTypeError() << "number_of_readable_tokens expects a string as its target, got " << target.GetTypeName();
    }

    const auto targetBuf = target.GetString().GetBounds();
    Y_ASSERT(IsUtf(targetBuf));

    i64 result = 0;
    auto checkPrintable = [&result](const TStringBuf token) {
        wchar32 rune;
        const unsigned char* curr = reinterpret_cast<const unsigned char*>(token.begin());
        const unsigned char* end = reinterpret_cast<const unsigned char*>(token.end());
        while (curr != end && RECODE_OK == ReadUTF8CharAndAdvance(rune, curr, end)) {
            if (IsAlphabetic(rune) || IsDigit(rune)) {
                ++result;
                return;
            }
        }
    };

    StringSplitter(targetBuf).SplitByFunc(CheckSpace).SkipEmpty().Consume(checkPrintable);
    return TValue::Integer(result);
}

TValue Upper(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    return StrUpper(ctx, globals, target);
}

TValue Join(const TCallCtx& ctx, const TGlobalsChain* globals,
            const TValue& target, const TValue& delimiter, const TValue& attribute) {
    if (attribute.IsNone()) {
        return StrJoin(ctx, globals, delimiter, target);
    }

    // TODO(a-square): consider refactoring when adding the map filter
    if (!attribute.IsString()) {
        ythrow TTypeError() << "join expects either none or a string as the attribute, got " << attribute.GetTypeName();
    }

    if (!target.IsList()) {
        ythrow TTypeError() << "join expects a list as the target, got " << target.GetTypeName();
    }

    const auto& targetList = target.GetList();
    const auto& attrStr = attribute.GetString();

    TValue::TList result(Reserve(targetList.size()));
    for (const auto& item : targetList) {
        result.push_back(GetAttrLoad(item, attrStr.GetBounds()));
    }
    return StrJoin(ctx, globals, delimiter, TValue::List(std::move(result)));
}

TValue JoinNumberWithDateTimeComponent(const TCallCtx& ctx, const TGlobalsChain* globals,
                                       int number, const TValue& dateTimeComponent,
                                       TStringBuf ruOnlyNumberPrefix, const TStringBuf lang) {
    if (lang == NLangs::Ru) {
        return Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(ruOnlyNumberPrefix),
                TValue::String(ToString(number)),
                dateTimeComponent
            }),
            TValue::String(" "),
            TValue::None()
        );
    } else {
        return Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(ToString(number)),
                dateTimeComponent
            }),
            TValue::String(" "),
            TValue::None()
        );
    }
}

TValue First(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return GetItemLoadInt(target, 0);
}

TValue Last(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return GetItemLoadInt(target, -1);
}

TValue Length(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Integer(GetLength(target));
}

TValue Replace(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& from, const TValue& to) {
    return StrReplace(ctx, globals, target, from, to);
}

TValue List(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    if (target.IsList()) {
        return target;
    }

    if (target.IsDict()) {
        return DictKeys(ctx, globals, target);
    }

    if (target.IsRange()) {
        const auto& range = target.GetRange();
        TValue::TList result(Reserve(range.GetSize()));
        for (const i64 item : range) {
            result.push_back(TValue::Integer(item));
        }
        return TValue::List(std::move(result));
    }

    // TODO(a-square): consider supporting string targets
    ythrow TTypeError() << "list got unexpected target type: " << target.GetTypeName();
}

TValue SplitBigNumber(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsInteger() || target.GetInteger() < 0) {
        ythrow TTypeError() << "split_big_number expects positive integer as the target, got " << target.GetTypeName();
    }
    TStringBuilder result;
    TString targetStr = ToString(target.GetInteger());

    static constexpr size_t nonSplitMaxNumberLength = 4;
    if (targetStr.size() <= nonSplitMaxNumberLength) {
        return TValue::String(targetStr);
    }

    TStringBuf targetStrBuf(targetStr);
    static constexpr size_t groupSize = 3;
    ui8 headSize = targetStrBuf.size() % groupSize;
    if (headSize) {
        result << targetStrBuf.Head(headSize);
    }
    for (size_t i = headSize; i < targetStrBuf.size(); i += groupSize) {
        if (i) {
            result << ' ';
        }
        result << targetStrBuf.SubStr(i, groupSize);
    }
    return TValue::String(result);
}

TValue String(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (target.IsString()) {
        return target;
    }

    // XXX(a-square): here we ignore string flags inside possible lists of strings,
    // maybe it's wrong
    return TValue::String(TStringBuilder{} << target);
}

TValue DictGet(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& key, const TValue& defValue) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "dict.get expects a dict as the target, got " << target.GetTypeName();
    }

    if (!key.IsString()) {
        ythrow TTypeError() << "dict.get expects a string as the key, got " << key.GetTypeName();
    }

    if (const auto* value = target.GetDict().FindPtr(key.GetString().GetStr())) {
        return *value;
    }

    return defValue;
}

TValue DictItems(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "dict.items expects a dict as the target, got " << target.GetTypeName();
    }

    const auto& dict = target.GetDict();

    // TODO(a-square): replace data copy with view creation
    TValue::TList result;
    result.reserve(dict.size());

    for (const auto& [key, value] : dict) {
        result.push_back(TValue::List({{TValue::String(key), value}}));
    }

    return TValue::List(std::move(result));
}

TValue DictKeys(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "dict.keys expects a dict as the target, got " << target.GetTypeName();
    }

    const auto& dict = target.GetDict();

    // TODO(a-square): replace data copy with view creation
    TValue::TList result;
    result.reserve(dict.size());

    for (const auto& [key, value] : dict) {
        result.push_back(TValue::String(key));
    }

    return TValue::List(std::move(result));
}

TValue DictUpdate(const TCallCtx&, const TGlobalsChain*, TValue target, const TValue& other) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "dict.update expects a dict as the target, got " << target.GetTypeName();
    }

    if (!other.IsDict()) {
        ythrow TTypeError() << "dict.update expects a dict as the other, got " << other.GetTypeName();
    }

    auto& targetDict = target.GetMutableDict();
    const auto& otherDict = other.GetDict();
    targetDict.reserve(targetDict.size() + otherDict.size());

    for (const auto& [key, value] : otherDict) {
        targetDict[key] = value;
    }

    return TValue::None();
}

TValue DictValues(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "dict.values expects a dict as the target, got " << target.GetTypeName();
    }

    const auto& dict = target.GetDict();

    // TODO(a-square): replace data copy with view creation
    TValue::TList result;
    result.reserve(dict.size());

    for (const auto& [key, value] : dict) {
        result.push_back(value);
    }

    return TValue::List(std::move(result));
}

TValue ListAppend(const TCallCtx&, const TGlobalsChain*, TValue target, const TValue& item) {
    if (!target.IsList()) {
        ythrow TTypeError() << "list.append expects a list as the target, got " << target.GetTypeName();
    }

    target.GetMutableList().push_back(item);
    return TValue::None();
}

TValue StrEndsWith(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& suffix) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.endswith expects a string as the target, got " << target.GetTypeName();
    }

    // XXX(a-square): here we don't take character flags into consideration, maybe it's wrong
    const TStringBuf str = target.GetString().GetBounds();

    if (suffix.IsString()) {
        return TValue::Bool(str.EndsWith(suffix.GetString().GetBounds()));
    }

    if (suffix.IsList()) {
        const auto& suffixList = suffix.GetList();
        for (const auto& suffixItem : suffixList) {
            if (!suffixItem.IsString()) {
                ythrow TTypeError() << "str.endswith expects a string or a list of strings as the suffix, got "
                                    << suffixItem.GetTypeName();
            }
            if (str.EndsWith(suffixItem.GetString().GetBounds())) {
                return TValue::Bool(true);
            }
        }

        return TValue::Bool(false);
    }

    ythrow TTypeError() << "str.endswith expects a string or a list of strings as the suffix, got "
                        << suffix.GetTypeName();
}

TValue StrJoin(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& items) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.join expects a string as the target, got " << target.GetTypeName();
    }

    if (!items.IsList()) {
        ythrow TTypeError() << "str.join expects a list as items, got " << items.GetTypeName();
    }

    const auto& targetText = target.GetString();

    TText result;

    bool first = true;
    for (const auto& item : items) {
        if (first) {
            first = false;
        } else {
            result.Append(targetText);
        }

        if (!item.IsString()) {
            ythrow TTypeError() << "str.join expects items to be strings, got " << item.GetTypeName();
        }
        result.Append(item.GetString());
    }

    return TValue::String(std::move(result));
}

TValue StrLower(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.lower expects a string as the target, got " << target.GetTypeName();
    }

    TText result;
    for (const auto& [str, flags] : target.GetString()) {
        result.Append(ToLowerUTF8(str), flags);
    }
    return TValue::String(std::move(result));
}

TValue StrLstrip(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& chars) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.lstrip expects a string as the target, got " << target.GetTypeName();
    }

    const auto& targetStr = target.GetString();

    if (chars.IsString()) {
        const TStringBuf charsBuf = chars.GetString().GetBounds();
        const TStringBuf targetBuf = targetStr.GetBounds();
        const TStringBuf strippedBuf = StripStringLeft(targetBuf, TUtf8CharsStripAdapter{charsBuf, targetBuf});
        return TValue::String(TText{targetStr, strippedBuf});
    }

    if (chars.IsNone()) {
        // TODO(a-square): strip ASCII whitespace okay? or do we need to strip all Unicode whitespace?
        return TValue::String(TText{targetStr, StripStringLeft(targetStr.GetBounds())});
    }

    ythrow TTypeError() << "str.lstrip expects a string or none as chars, got " << chars.GetTypeName();
}

TValue StrReplace(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& substr,
                  const TValue& repl) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.replace expects a string as the target, got " << target.GetTypeName();
    }

    if (!substr.IsString()) {
        ythrow TTypeError() << "str.replace expects a string as the substring to replace, got " << substr.GetTypeName();
    }

    if (!repl.IsString()) {
        ythrow TTypeError() << "str.replace expects a string as the replacement string, got " << repl.GetTypeName();
    }

    const TStringBuf substStr = substr.GetString().GetBounds();
    const TStringBuf replStr = repl.GetString().GetBounds();

    if (!substStr) {
        // emulate Python's handling of empty substitution string
        return TValue::String(PadString(target.GetString().GetStr(), replStr));
    }

    // XXX(a-square): we reset the string flags, maybe it's wrong
    auto targetStr = target.GetString().GetStr();
    SubstGlobal(targetStr, substStr, replStr);
    return TValue::String(std::move(targetStr));
}

TValue StrRstrip(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& chars) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.rstrip expects a string as the target, got " << target.GetTypeName();
    }

    const auto& targetStr = target.GetString();

    if (chars.IsString()) {
        const TStringBuf charsBuf = chars.GetString().GetBounds();
        const TStringBuf targetBuf = targetStr.GetBounds();
        const TStringBuf strippedBuf = StripStringRight(targetBuf, TUtf8CharsStripAdapter{charsBuf, targetBuf});
        return TValue::String(TText{targetStr, strippedBuf});
    }

    if (chars.IsNone()) {
        // TODO(a-square): strip ASCII whitespace okay? or do we need to strip all Unicode whitespace?
        return TValue::String(TText{targetStr, StripStringRight(targetStr.GetBounds())});
    }

    ythrow TTypeError() << "str.rstrip expects a string or none as chars, got " << chars.GetTypeName();
}

TValue StrSplit(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& sep, const TValue& num) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.split expects a string as the target, got " << target.GetTypeName();
    }

    if (!sep.IsString() && !sep.IsNone()) {
        ythrow TTypeError() << "str.split expects a string or none as the separator, got " << sep.GetTypeName();
    }

    if (!num.IsInteger() && !num.IsNone()) {
        ythrow TTypeError() << "str.split expects a string or none as the number of splits, got " << sep.GetTypeName();
    }

    // XXX(a-square): we ignore separator's flags, maybe it's wrong
    const TText& targetText = target.GetString();
    const TStringBuf targetBuf = targetText.GetBounds();
    const TStringBuf sepBuf = sep.IsString() ? sep.GetString().GetBounds() : TStringBuf{};
    const i64 limit = num.IsInteger() ? num.GetInteger() + 1 : ::Max<i64>(); // max number of parts

    TValue::TList result;
    auto store = [&targetText, &result](const TStringBuf part) {
        result.push_back(TValue::String(TText{targetText, part})); // clip the initial text into the part
    };

    // Python's str.split has two very distinct modes, based on whether the splitting string is given
    if (sepBuf) {
        StringSplitter(targetBuf).SplitByString(sepBuf).Limit(limit).Consume(store);
    } else {
        // TODO(a-square): splitting by ASCII whitespace only, should we go full Unicode?
        StringSplitter(targetBuf).SplitByFunc(CheckSpace).SkipEmpty().Limit(limit).Consume(store);
    }

    return TValue::List(std::move(result));
}

TValue StrStartsWith(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& prefix) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.startswith expects a string as the target, got " << target.GetTypeName();
    }

    // XXX(a-square): here we don't take character flags into consideration, maybe it's wrong
    const TStringBuf str = target.GetString().GetBounds();

    if (prefix.IsString()) {
        return TValue::Bool(str.StartsWith(prefix.GetString().GetBounds()));
    } else if (prefix.IsList()) {
        const auto& prefixList = prefix.GetList();
        for (const auto& prefixItem : prefixList) {
            if (!prefixItem.IsString()) {
                ythrow TTypeError() << "str.startswith expects a string or a list of strings as the prefix, got "
                                    << prefixItem.GetTypeName();
            }
            if (str.StartsWith(prefixItem.GetString().GetBounds())) {
                return TValue::Bool(true);
            }
        }

        return TValue::Bool(false);
    } else {
        ythrow TTypeError() << "str.startswith expects a string or a list of strings as the prefix, got "
                            << prefix.GetTypeName();
    }
}

TValue StrStrip(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& chars) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.strip expects a string as the target, got " << target.GetTypeName();
    }

    const auto& targetStr = target.GetString();

    if (chars.IsString()) {
        const TStringBuf charsStr = chars.GetString().GetBounds();
        const TStringBuf targetBuf = targetStr.GetBounds();
        const TStringBuf strippedBuf = StripString(targetBuf, TUtf8CharsStripAdapter{charsStr, targetBuf});
        return TValue::String(TText{targetStr, strippedBuf});
    }

    if (chars.IsNone()) {
        // TODO(a-square): strip ASCII whitespace okay? or do we need to strip all Unicode whitespace?
        return TValue::String(TText{targetStr, StripString(targetStr.GetBounds())});
    }

    ythrow TTypeError() << "str.strip expects a string or none as chars, got " << chars.GetTypeName();
}

TValue StrUpper(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "str.upper expects a string as the target, got " << target.GetTypeName();
    }

    TText result;
    for (const auto& [str, flags] : target.GetString()) {
        result.Append(ToUpperUTF8(str), flags);
    }
    return TValue::String(std::move(result));
}

TValue TestDefined(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(!target.IsUndefined());
}

TValue TestUndefined(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(target.IsUndefined());
}

TValue TestDivisibleBy(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& num) {
    return TValue::Bool(!TruthValue(NOperators::Mod(target, num)));
}

TValue TestEven(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(!TruthValue(NOperators::Mod(target, TValue::Integer(2))));
}

TValue TestOdd(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(TruthValue(NOperators::Mod(target, TValue::Integer(2))));
}

TValue TestEq(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::Equals(target, other);
}

TValue TestGe(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::GreaterEq(target, other);
}

TValue TestGt(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::Greater(target, other);
}

TValue TestIn(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::ValueIn(target, other);
}

TValue TestLe(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::LessEq(target, other);
}

TValue TestLt(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::Less(target, other);
}

TValue TestNe(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& other) {
    return NOperators::NotEquals(target, other);
}

TValue TestIterable(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    // TODO(a-square): consider making string iterable
    return TValue::Bool(target.IsUndefined() || target.IsList() || target.IsDict() || target.IsRange());
}

TValue TestLower(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TestCharCase(target, IsLower, IsUpper, "lower");
}

TValue TestUpper(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TestCharCase(target, IsUpper, IsLower, "upper");
}

TValue TestMapping(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(target.IsDict());
}

TValue TestNone(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(target.IsNone());
}

TValue TestNumber(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(target.IsInteger() || target.IsDouble());
}

TValue TestSequence(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(TestSequenceImpl(target));
}

TValue TestString(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    return TValue::Bool(target.IsString());
}

TValue CityPrepcase(const TCallCtx& ctx, const TGlobalsChain*, const TValue& geo) {
    if (!geo.IsDict()) {
        ythrow TTypeError() << "geo must be a dict, got a " << geo.GetTypeName();
    }

    const auto& geoDict = geo.GetDict();
    const auto cityCasesIter = geoDict.find("city_cases");

    if (!cityCasesIter.IsEnd()) {
        const auto& cityCases = cityCasesIter->second;
        if (!cityCases.IsDict()) {
            ythrow TTypeError() << "city_cases must be a geoDict, got a " << cityCases.GetTypeName();
        }

        const auto& cityCasesDict = cityCases.GetDict();
        const auto prepositionIter = cityCasesDict.find("preposition");
        const auto prepositionalIter = cityCasesDict.find("prepositional");
        if (!prepositionIter.IsEnd() && !prepositionalIter.IsEnd()) {
            return TValue::String(Sprintf("%s %s", prepositionIter->second.GetString().GetData(), prepositionalIter->second.GetString().GetData()));
        }
    }

    const auto cityIter = geoDict.find("city");
    if (!cityIter.IsEnd()) {
        const TStringBuf lang = ExtractLanguage(ctx);
        return TValue::String(Sprintf("%s %s", GetPrepCaseByLang(lang).InCity.c_str(), cityIter->second.GetString().GetData()));
    }

    ythrow TValueError() << "geo does not contain city info " << geo;
}

TValue Emojize(const TCallCtx&, const TGlobalsChain*, const TValue& value) {
    // XXX(a-square): here we assume that our string doesn't have meaningful flags.
    // This is true for all current use cases, but may change in the future.
    const auto& str = value.GetString().GetStr();

    // all emoji codes look like ":foo:" so no emoji code can be shorter than 3 chars
    if (str.size() < 3) {
        return value;
    }

    // first try the common case (the whole str is just one emoji)
    if (str.front() == ':' && str.back() == ':') {
        if (const auto* ptr = NImpl::CODES_TO_EMOJI.FindPtr(str)) {
            return TValue::String(*ptr);
        }
    }

    TStringBuf in(str);
    TStringBuilder out;

    const auto distance = [in](size_t from, size_t to) {
        if (to == TStringBuf::npos) {
            to = in.size();
        }
        return to - from;
    };

    size_t curr = 0;
    while (curr != in.size()) {
        const auto cbegin = in.find(':', curr);
        out << in.substr(curr, distance(curr, cbegin));
        if (cbegin == TStringBuf::npos) {
            break;
        }

        const auto cend = in.find(':', cbegin + 1);
        if (cend == TStringBuf::npos) {
            out << in.substr(cbegin);
            break;
        }

        const auto code = in.substr(cbegin, cend - cbegin + 1);
        if (const auto* emoji = NImpl::CODES_TO_EMOJI.FindPtr(code)) {
            out << *emoji;
            curr = cend + 1;
        } else {
            out << in.substr(cbegin, cend - cbegin);
            curr = cend;
        }
    }

    return TValue::String(out);
}

TValue MusicTitleShorten(const TCallCtx& /* ctx */, const TGlobalsChain* /* globals */, const TValue& value) {
    if (!value.IsString()) {
        ythrow TTypeError() << "music_title_shorten expects its target to be a string, got " << value.GetTypeName();
    }

    static constexpr TStringBuf delimChars = ",;";

    const auto str = value.GetString().GetBounds();
    const auto noParensStr = StripParenPairs(str);
    if (!noParensStr) {
        return TValue::String(StripString(str));
    }

    const auto delim = std::find_first_of(noParensStr.begin(), noParensStr.end(),
                                          delimChars.begin(), delimChars.end());
    const auto beforeDelim = StripString(TStringBuf{noParensStr.begin(), delim});
    if (!beforeDelim) {
        return TValue::String(noParensStr);
    }

    return TValue::String(beforeDelim);
}

TValue TrimWithEllipsis(const TCallCtx&, const TGlobalsChain*, const TValue& value, const TValue& widthLimit) {
    // NOTE(a-square): this is the direct port of trim_with_ellipsis
    // from alice/vins/core/vins_core/utils/strings.py,
    // which has several quirks that make it impossible to just plug
    // some function already in the util instead.
    //
    // The function iterates over space-delimited words in the string,
    // counting the number of Unicode symbols seen so far.
    // When this number becomes greater than or equal to widthLimit,
    // it stops and returns the string.
    // If there are some symbols left unconsumed, it trims the end of the string
    // of any punctuation and adds "..." instead.
    // If all symbols were consumed, it returns the string as is.
    // It always returns empty strings without modification.
    if (value.IsUndefined() || value.IsNone()) {
        return value;
    }

    if (!value.IsString()) {
        ythrow TTypeError() << "trim_with_ellipsis: value must be a string, none, or undefined";
    }

    // We ignore text flags in this filter because in the original implementation,
    // trimming could've led to unterminated XML tags and a runtime error.
    // Thus we just assume that our string doesn't have meaningful flags.
    const auto& str = value.GetString().GetStr();
    if (!str) {
        return value;
    }

    if (!widthLimit.IsInteger()) {
        ythrow TTypeError() << "trim_with_ellipsis: widthLimit must be an integer";
    }
    size_t limit = static_cast<size_t>(::Max(0L, widthLimit.GetInteger()));

    // accepted part of the string
    auto begin = str.begin(); // always points to the beginning of the string
    auto end = str.begin();   // always points to the end of the last parsed token

    // length of the string accepted so far
    size_t total = 0;
    for (auto it : StringSplitter(str).Split(' ')) {
        Y_ASSERT(total == GetNumberOfUTF8Chars(TStringBuf(begin, end)));

        // XXX(a-square): reproducing the length bug
        // in the original trim_with_ellipsis here,
        // consider fixing it after the migration is complete
        if (total >= limit) {
            auto fragment = StripStringRight(TStringBuf(begin, end),
                                             [](TStringBuf::const_iterator strIt) { return IsAsciiPunct(*strIt); });
            return TValue::String(TStringBuilder() << fragment << TStringBuf("..."));
        }

        size_t chunkLength = GetNumberOfUTF8Chars(it.Token());
        if (it.TokenStart() != begin) {
            // if it's not the first token, account for the splitting character
            total += 1;
        }
        total += chunkLength;
        end = it.TokenDelim(); // this, not TokenEnd(), is the iterator pointing to the end of it.Token()'s
    }

    // if we never returned in the loop, it means the entire string fits
    return value;
}

TValue OnlyText(const TCallCtx&, const TGlobalsChain*, const TValue& value) {
    if (!value.IsString()) {
        ythrow TTypeError() << "only_text expects a string, got " << value.GetTypeName();
    }

    return TValue::String(TText{value.GetString().ExtractSpans(TText::EFlag::Text), TText::EFlag::Text});
}

TValue OnlyVoice(const TCallCtx&, const TGlobalsChain*, const TValue& value) {
    if (!value.IsString()) {
        ythrow TTypeError() << "only_voice expects a string, got " << value.GetTypeName();
    }

    return TValue::String(TText{value.GetString().ExtractSpans(TText::EFlag::Voice), TText::EFlag::Voice});
}

TValue VoiceAndTextResult(const TCallCtx& ctx, const TGlobalsChain* globals,
                          const TValue& textResult, const TValue& voiceResult) {
    return TValue::Dict({
        {"voice", OnlyVoice(ctx, globals, voiceResult)},
        {"text", OnlyText(ctx, globals, textResult)}
    });
}

TValue Pluralize(const TCallCtx& ctx, const TGlobalsChain*,
                 const TValue& target, const TValue& number, const TValue& inflCase) {
    if (!target.IsString()) {
        ythrow TTypeError() << "pluralize expects a string as its target, got " << target.GetTypeName();
    }

    const TStringBuf lang = ExtractLanguage(ctx);
    if (lang != NLangs::Ru) {
        return target;
    }

    std::variant<double, ui64> numberVariant;
    if (number.IsDouble()) {
        const double numberDouble = number.GetDouble();
        if (numberDouble < 0) {
            ythrow TValueError() << "pluralize expects a non-negative number as the number, got " << numberDouble;
        }
        numberVariant = numberDouble;
    } else if (const auto maybeInteger = ToInteger(number)) {
        if (*maybeInteger < 0) {
            ythrow TValueError() << "pluralize expects a non-negative number as the number, got " << *maybeInteger;
        }
        numberVariant = static_cast<ui64>(*maybeInteger);
    } else {
        ythrow TTypeError() << "pluralize expects a number as the number, got " << number.GetTypeName();
    }

    if (!inflCase.IsString()) {
        ythrow TTypeError() << "pluralize expects a string as the case, got " << inflCase.GetTypeName();
    }

    return TValue::String(PluralizeWords(GetInflector(),
                                         target.GetString().GetStr(),
                                         numberVariant,
                                         inflCase.GetString().GetStr()));
}

TValue Singularize(const TCallCtx&, const TGlobalsChain*,
                   const TValue& target, const TValue& number) {
    if (!target.IsString()) {
        ythrow TTypeError() << "singularize expects a string as its target, got " << target.GetTypeName();
    }

    if (const auto maybeInteger = ToInteger(number)) {
        if (*maybeInteger < 0) {
            ythrow TValueError() << "singularize expects a non-negative integer as the number, got " << *maybeInteger;
        }
        return TValue::String(SingularizeWords(target.GetString().GetBounds(), static_cast<ui64>(*maybeInteger)));
    }

    ythrow TTypeError() << "singularize expects a non-negative integer as the number, got " << number.GetTypeName();
}

TValue HumanTime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "target must be a dict, got " << target.GetTypeName();
    }

    if (!timezone.IsString() && !timezone.IsNone()) {
        ythrow TTypeError() << "timezone must be a string or none, got " << timezone.GetTypeName();
    }

    const TValue& tz = timezone.IsString() ? timezone : GetAttrLoad(target, NDateKeys::Timezone);
    TValue date = ConvertTimezone(target, tz);

    return TValue::String(ConstructHumanTime(ctx, globals, date));
}

TValue HumanTimeRaw(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "target must be a dict, got " << target.GetTypeName();
    }

    return TValue::String(ConstructHumanTime(ctx, globals, target));
}

TValue HumanDate(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "target must be a dict, got " << target.GetTypeName();
    }

    if (!timezone.IsString() && !timezone.IsNone()) {
        ythrow TTypeError() << "timezone must be a string or none, got " << timezone.GetTypeName();
    }

    const TValue& tz = timezone.IsString() ? timezone : GetAttrLoad(target, NDateKeys::Timezone);
    TValue date = ConvertTimezone(target, tz);

    long dateYear = GetAttrLoad(date, NDateKeys::Year).GetInteger();
    long currentYear = GetNowCivilTime(tz).Year + NTime::YEAR_OFFSET;

    return TValue::String(ConstructHumanDate(ctx, globals, date, dateYear != currentYear));
}

//
// @human_day_rel
// Usage:
// {{ date | human_day_rel }}
//
// date - объект типа create_date_safe()
//    Печатает дату в формате
//        * в пределах +-2 дней - позавчера/вчера/сегодня/завтра/послезавтра
//        * в остальных случаях - в формате 12 марта [1980 года] (год пишется, если он не совпадает с текущим)
//
TValue HumanDayRel(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone, const TValue& mockedTime) {
    if (!target.IsDict()) {
        ythrow TTypeError() << "target must be a dict, got " << target.GetTypeName();
    }

    if (!timezone.IsString() && !timezone.IsNone()) {
        ythrow TTypeError() << "timezone must be a string or none, got " << timezone.GetTypeName();
    }

    const TValue& tz = timezone.IsString() ? timezone : GetAttrLoad(target, NDateKeys::Timezone);
    TValue date = ConvertTimezone(target, tz);

    long year = GetAttrLoad(date, NDateKeys::Year).GetInteger();
    long month = GetAttrLoad(date, NDateKeys::Month).GetInteger();
    long day = GetAttrLoad(date, NDateKeys::Day).GetInteger();

    TMaybe<TInstant> mockedTimeValue;
    if (mockedTime.IsInteger()) {
        mockedTimeValue.ConstructInPlace(TInstant::MicroSeconds(mockedTime.GetInteger()));
    }
    auto nowTime = GetNowCivilTime(tz, mockedTimeValue);
    const TStringBuf lang = ExtractLanguage(ctx);
    const TRelativeDay& relativeDay = GetRelativeDayByLang(lang);

    for (int dayDiff : TRelativeDay::SupportedRelative) {
        auto diffedTime = nowTime;
        diffedTime.Add(NDatetime::TSimpleTM::F_DAY, dayDiff);

        if (diffedTime.MDay == day && diffedTime.Mon + 1 == month && diffedTime.Year + NTime::YEAR_OFFSET == year) {
            return TValue::String(relativeDay.GetRelativeDayName(dayDiff));
        }
    }

    return HumanDate(ctx, globals, date, timezone);
}

//
// @is_human_day_rel
// Usage:
// {% if date | is_human_day_rel %}
//
// date - объект типа create_date_safe()
// Вовзращает true, если печать даты будет в относительном формате (см human_day_rel)
// Возвращает false, если печать даты произойдет с указанием числа, месяца и (опционально) года
//
TValue IsHumanDayRel(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& target, const TValue& timezone, const TValue& mockedTime) {

    Y_UNUSED(ctx);
    Y_UNUSED(globals);

    Y_ENSURE(target.IsDict(), "target must be a dict, got " << target.GetTypeName());
    Y_ENSURE(timezone.IsString() || timezone.IsNone(), "timezone must be a string or none, got " << timezone.GetTypeName());

    const TValue& tz = timezone.IsString() ? timezone : GetAttrLoad(target, NDateKeys::Timezone);
    TValue date = ConvertTimezone(target, tz);

    const long year = GetAttrLoad(date, NDateKeys::Year).GetInteger();
    const long month = GetAttrLoad(date, NDateKeys::Month).GetInteger();
    const long day = GetAttrLoad(date, NDateKeys::Day).GetInteger();

    TMaybe<TInstant> mockedTimeValue;
    if (mockedTime.IsInteger()) {
        mockedTimeValue.ConstructInPlace(TInstant::MicroSeconds(mockedTime.GetInteger()));
    }
    const auto nowTime = GetNowCivilTime(tz, mockedTimeValue);

    for (int dayDiff : TRelativeDay::SupportedRelative) {
        auto diffedTime = nowTime;
        diffedTime.Add(NDatetime::TSimpleTM::F_DAY, dayDiff);

        if (diffedTime.MDay == day && diffedTime.Mon + 1 == month && diffedTime.Year + NTime::YEAR_OFFSET == year) {
            return TValue::Bool(true);
        }
    }
    return TValue::Bool(false);
}

TValue ClientActionDirective(const TCallCtx&, const TGlobalsChain*,
                             const TValue& name, const TValue& payload, const TValue& type, const TValue& subName) {
    return TValue::Dict({
        {NClientActionDirectiveKeys::Name, name},
        {NClientActionDirectiveKeys::Payload, payload},
        {NClientActionDirectiveKeys::Type, type},
        {NClientActionDirectiveKeys::SubName, subName},
    });
}

TValue CreateDateSafe(const TCallCtx&, const TGlobalsChain*,
                      const TValue& year, const TValue& month, const TValue& day) {
    const auto maybeDate = ToDatetime(year, month, day);
    if (!maybeDate) {
        return TValue::None();
    }

    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(maybeDate->year())},
        {NDateKeys::Month, TValue::Integer(maybeDate->month())},
        {NDateKeys::Day, TValue::Integer(maybeDate->day())},
        {NDateKeys::Hour, TValue::Integer(maybeDate->hour())},
        {NDateKeys::Minute, TValue::Integer(maybeDate->minute())},
        {NDateKeys::Second, TValue::Integer(maybeDate->second())},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, TValue::None()},
    });
}

// this function does not take 'tzinfo' field as its counterpart
// otherwise, we would have problems with tzinfo format in strftime
// see example: datetime(2005, 2, 7, tzinfo=parse_tz('Europe/Moscow')).strftime('%z') = +0230
TValue Datetime(const TCallCtx&, const TGlobalsChain*, const TValue& year, const TValue& month, const TValue& day,
                const TValue& hour, const TValue& minute, const TValue& second, const TValue& microsecond) {
    const auto maybeDate = ToDatetime(year, month, day, hour, minute, second, microsecond);
    if (!maybeDate) {
        ythrow TTypeError() << "wrong format for datetime";
    }

    if (microsecond.GetInteger() < 0 || microsecond.GetInteger() > 999999) {
        ythrow TValueError() << "microsecond must be in the 0..999999 range";
    }

    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(maybeDate->year())},
        {NDateKeys::Month, TValue::Integer(maybeDate->month())},
        {NDateKeys::Day, TValue::Integer(maybeDate->day())},
        {NDateKeys::Hour, TValue::Integer(maybeDate->hour())},
        {NDateKeys::Minute, TValue::Integer(maybeDate->minute())},
        {NDateKeys::Second, TValue::Integer(maybeDate->second())},
        {NDateKeys::Microsecond, microsecond},
        {NDateKeys::Timezone, TValue::None()},
    });
}

TValue TimestampToDatetime(const TCallCtx&, const TGlobalsChain*, const TValue& timestamp, const TValue& timezone) {
    if (!timestamp.IsInteger()) {
        ythrow TTypeError() << "timestamp must be an integer, got " << timestamp.GetTypeName();
    }

    if (!timezone.IsString()) {
        ythrow TTypeError() << "timezone must be a string, got a " << timezone.GetTypeName();
    }
    const auto utcDate = cctz::civil_second() + timestamp.GetInteger();
    const auto absTime = cctz::convert(utcDate, cctz::utc_time_zone());
    cctz::time_zone tz;
    if (!cctz::load_time_zone(timezone.GetString().GetStr(), &tz)) {
        ythrow NDatetime::TInvalidTimezone();
    }
    const auto date = cctz::convert(absTime, tz);
    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(date.year())},
        {NDateKeys::Month, TValue::Integer(date.month())},
        {NDateKeys::Day, TValue::Integer(date.day())},
        {NDateKeys::Hour, TValue::Integer(date.hour())},
        {NDateKeys::Minute, TValue::Integer(date.minute())},
        {NDateKeys::Second, TValue::Integer(date.second())},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, timezone},
    });
}

TValue DatetimeStrftime(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& datetimeFormat) {
    if (!datetimeFormat.IsString()) {
        ythrow TTypeError() << "required argument 'datetime_format' must be a string, got " << datetimeFormat.GetTypeName();
    }

    const auto maybeDatetime = ToDatetime(target, /* switchTimezone = */ false);
    if (!maybeDatetime) {
        ythrow TTypeError() << "datetime object was expected";
    }

    NDatetime::TTimeZone timezone = GetTimezone(GetAttrLoad(target, NDateKeys::Timezone));
    const auto pattern  = datetimeFormat.GetString().GetStr();
    return TValue::String(NDatetime::Format(pattern, maybeDatetime.GetRef(), timezone));
}

TValue DatetimeStrptime(const TCallCtx&, const TGlobalsChain*, const TValue& dateString, const TValue& dateFormat) {
    if (!dateString.IsString()) {
        ythrow TTypeError() << "date_string must be a string, got a " << dateString.GetTypeName();
    }

    if (!dateFormat.IsString()) {
        ythrow TTypeError() << "date_format must be a string, got a " << dateFormat.GetTypeName();
    }
    const cctz::time_zone tz = cctz::utc_time_zone();
    std::chrono::system_clock::time_point tp;
    const auto pattern = dateFormat.GetString().GetStr();
    const auto timeData = dateString.GetString().GetStr();
    if (!cctz::parse(pattern, timeData, tz, &tp)) {
        ythrow TValueError() << "time data " << timeData << " does not match format " << pattern;
    }

    const auto date = cctz::convert(tp, tz);
    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(date.year())},
        {NDateKeys::Month, TValue::Integer(date.month())},
        {NDateKeys::Day, TValue::Integer(date.day())},
        {NDateKeys::Hour, TValue::Integer(date.hour())},
        {NDateKeys::Minute, TValue::Integer(date.minute())},
        {NDateKeys::Second, TValue::Integer(date.second())},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, TValue::None()},
    });
}


TValue ParseDt(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    if (!target.IsString()) {
        ythrow TTypeError() << "datetime_string must be a string, got a " << target.GetTypeName();
    }

    const auto tp = ParseDatetime(target);
    const auto tz = cctz::utc_time_zone();
    const auto date = cctz::convert(tp, tz);
    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(date.year())},
        {NDateKeys::Month, TValue::Integer(date.month())},
        {NDateKeys::Day, TValue::Integer(date.day())},
        {NDateKeys::Hour, TValue::Integer(date.hour())},
        {NDateKeys::Minute, TValue::Integer(date.minute())},
        {NDateKeys::Second, TValue::Integer(date.second())},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, TValue::None()},
    });
}

TValue DatetimeIsoweekday(const TCallCtx&, const TGlobalsChain*, const TValue& target) {
    const auto maybeDatetime = ToDatetime(target);
    if (!maybeDatetime) {
        ythrow TTypeError() << "datetime object was expected";
    }

    const auto day = NDatetime::TCivilDay(maybeDatetime.GetRef());
    const auto weekday = NDatetime::GetWeekday(day);
    return TValue::Integer(ToUnderlying(weekday) + 1);
}

TValue ParseTz(const TCallCtx&, const TGlobalsChain*, const TValue& timezoneFormat) {
    if (!timezoneFormat.IsString()) {
        ythrow TTypeError() << "timezone_format must be a string, got a " << timezoneFormat.GetTypeName();
    }
    const auto pattern = timezoneFormat.GetString().GetStr();
    NDatetime::GetTimeZone(pattern);
    return TValue::String(pattern);
}

TValue Localize(const TCallCtx&, const TGlobalsChain*, const TValue& target, const TValue& datetime) {
    const auto maybeDatetime = ToDatetime(datetime);
    if (!maybeDatetime) {
        ythrow TTypeError() << "datetime object was expected";
    }
    const auto microsecond = GetAttrLoad(datetime, NDateKeys::Microsecond);

    if (!target.IsString()) {
        ythrow TTypeError() << "timezone object must be a string, got a " << target.GetTypeName();
    }

    const auto pattern = target.GetString().GetStr();
    NDatetime::GetTimeZone(pattern);

    if (!GetAttrLoad(datetime, NDateKeys::Timezone).IsNone()) {
        ythrow TValueError() << "not naive datetime (timezone is already set)";
    }

    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(maybeDatetime->year())},
        {NDateKeys::Month, TValue::Integer(maybeDatetime->month())},
        {NDateKeys::Day, TValue::Integer(maybeDatetime->day())},
        {NDateKeys::Hour, TValue::Integer(maybeDatetime->hour())},
        {NDateKeys::Minute, TValue::Integer(maybeDatetime->minute())},
        {NDateKeys::Second, TValue::Integer(maybeDatetime->second())},
        {NDateKeys::Microsecond, microsecond},
        {NDateKeys::Timezone, TValue::String(pattern)},
    });
}

class TPluralizeTagRegexp {
public:
    TPluralizeTagRegexp() {
        TStringBuilder regexpBuilder;
        regexpBuilder << "(\\d[\\d\\s]*)\\s(";
        ui32 position = 0;
        for (const auto& [tag, word] : GetUnitsTagsByLang(NLangs::Ru)) {
            if (position) {
                regexpBuilder << "|";
            }
            regexpBuilder << tag;
            position++;
        }
        Regexp = std::make_unique<re2::RE2>(regexpBuilder << ")");
    }

    const re2::RE2* GetRegexp() const {
        return Regexp.get();
    }

    static TPluralizeTagRegexp* GetInstance() {
        return Singleton<TPluralizeTagRegexp>();
    }
private:
    std::unique_ptr<re2::RE2> Regexp;
};

TValue PluralizeTag(const TCallCtx& ctx, const TGlobalsChain*, const TValue& target, const TValue& inflCase) {
    if (!target.IsString()) {
        ythrow TTypeError() << "pluralize tag expects a string as its target, got " << target.GetTypeName();
    }

    const TStringBuf lang = ExtractLanguage(ctx);
    if (lang != NLangs::Ru) {
        return target;
    }

    if (!inflCase.IsString()) {
        ythrow TTypeError() << "pluralize expects a string as the case, got " << inflCase.GetTypeName();
    }

    const auto& inputString = target.GetString().GetBounds();
    const re2::RE2& regexp = *TPluralizeTagRegexp::GetInstance()->GetRegexp();

    re2::StringPiece output;
    re2::StringPiece numberOutput;
    re2::StringPiece input(inputString.data(), inputString.size());

    if (RE2::PartialMatch(input, regexp, &numberOutput, &output)) {
        TString numberStr;
        numberStr.resize(numberOutput.size());
        numberStr.erase(std::remove_copy_if(numberOutput.begin(), numberOutput.end(), numberStr.begin(), CheckSpace),
                        numberStr.end());
        ui64 number;
        if (!TryFromString<ui64>(numberStr, number)) {
            ythrow TTypeError() << "pluralize expects a number as the number, got " << numberStr;
        }

        const auto& unitsTagsMapping = GetUnitsTagsByLang(lang);
        const auto& inflectedValue = PluralizeWords(GetInflector(),
                                                    unitsTagsMapping.at(TStringBuf{output.data(), output.size()}),
                                                    number,
                                                    inflCase.GetString().GetStr());

        TStringBuf prefix{input.begin(), output.begin()};
        TStringBuf suffix{output.end(), input.end()};
        return TValue::String(TStringBuilder() << prefix << inflectedValue << suffix);
    }
    return target;
}

TValue Randuniform(const TCallCtx& ctx, const TGlobalsChain*, const TValue& from, const TValue& to) {
    auto fromDouble = ToDouble(from);
    if (!fromDouble) {
        ythrow TTypeError() << "randuniform accepts a value convertible to a double, got " << from.GetTypeName();
    }
    auto toDouble = ToDouble(to);
    if (!toDouble) {
        ythrow TTypeError() << "randuniform accepts a value convertible to a double, got " << to.GetTypeName();
    }
    return TValue::Double(ctx.Rng.RandomDouble(*fromDouble, *toDouble));
}

TValue ServerActionDirective(const TCallCtx&, const TGlobalsChain*,
                             const TValue& name, const TValue& payload,
                             const TValue& type, const TValue& ignoreAnswer) {
    return TValue::Dict({
        {NServerActionDirectiveKeys::Name, name},
        {NServerActionDirectiveKeys::Payload, payload},
        {NServerActionDirectiveKeys::Type, type},
        {NServerActionDirectiveKeys::IgnoreAnswer, ignoreAnswer},
    });
}

TValue AddHours(const TCallCtx&, const TGlobalsChain*, const TValue& datetime, const TValue& hours) {
    if (!datetime.IsDict()) {
        ythrow TTypeError() << "datetime must be a dict, got a " << datetime.GetTypeName();
    }

    if (!hours.IsInteger()) {
        ythrow TTypeError() << "hours must be an integer, got a " << hours.GetTypeName();
    }

    TMaybe<NDatetime::TCivilSecond> civilDate = ToDatetime(datetime);
    if (!civilDate) {
        ythrow TTypeError() << "can't parse datetime";
    }

    (*civilDate) += TDuration::Hours(hours.GetInteger()).Seconds();
    return TValue::Dict({
        {NDateKeys::Year, TValue::Integer(civilDate->year())},
        {NDateKeys::Month, TValue::Integer(civilDate->month())},
        {NDateKeys::Day, TValue::Integer(civilDate->day())},
        {NDateKeys::Hour, TValue::Integer(civilDate->hour())},
        {NDateKeys::Minute, TValue::Integer(civilDate->minute())},
        {NDateKeys::Second, TValue::Integer(civilDate->second())},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, GetAttrLoad(datetime, NDateKeys::Timezone)},
    });
}

TValue CeilSeconds(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& timeUnits, const TValue& aggressive) {
    int hours = 0;
    if (auto hoursVal = GetAttrLoad(timeUnits, "hours"); hoursVal.IsInteger()) {
        hours = hoursVal.GetInteger();
    }

    int minutes = 0;
    if (auto minutesVal = GetAttrLoad(timeUnits, "minutes"); minutesVal.IsInteger()) {
        minutes = minutesVal.GetInteger();
    }

    int seconds = 0;
    if (auto secondsVal = GetAttrLoad(timeUnits, "seconds"); secondsVal.IsInteger()) {
        seconds = secondsVal.GetInteger();
    }

    if (seconds == 0) {
        return timeUnits;
    }

    if (hours == 0 && !aggressive.GetBool()) {
        return timeUnits;
    }

    return NormalizeTimeUnits(ctx, globals, TValue::Dict({
        {"hours", TValue::Integer(hours)},
        {"minutes", TValue::Integer(minutes + 1)}
    }));
}

TValue NormalizeTimeUnits(const TCallCtx& /* ctx */, const TGlobalsChain* /* globals */, const TValue& timeUnits) {
    int hours = 0;
    if (auto hoursVal = GetAttrLoad(timeUnits, "hours"); hoursVal.IsInteger()) {
        hours = hoursVal.GetInteger();
    }

    int minutes = 0;
    if (auto minutesVal = GetAttrLoad(timeUnits, "minutes"); minutesVal.IsInteger()) {
        minutes = minutesVal.GetInteger();
    }

    int seconds = 0;
    if (auto secondsVal = GetAttrLoad(timeUnits, "seconds"); secondsVal.IsInteger()) {
        seconds = secondsVal.GetInteger();
    }

    int secondsTotal = hours * 3600 + minutes * 60 + seconds;
    return TValue::Dict({
        {"hours", TValue::Integer(secondsTotal / 3600)},
        {"minutes", TValue::Integer((secondsTotal / 60) % 60)},
        {"seconds", TValue::Integer(secondsTotal % 60)},
    });
}

TValue RenderWeekdayType(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& weekday) {
    const auto weekdaysValue = GetAttrLoad(weekday, "weekdays");
    TVector<int> weekdays;
    if (!weekdaysValue.IsList() || weekdaysValue.GetList().empty()) {
        return TValue::String("");
    }

    for (auto& dayValue : weekdaysValue.GetList()) {
        weekdays.emplace_back(dayValue.GetInteger());
    }

    const auto& repeatedValue = GetAttrLoad(weekday, "repeat");
    const bool repeated = repeatedValue.IsBool() && repeatedValue.GetBool();

    const TStringBuf lang = ExtractLanguage(ctx);
    const TDaysPeriod& daysPeriod = GetDaysPeriodByLang(lang);

    if (IsWeekday(weekdays)) {
        return TValue::String(
            repeated ?
            daysPeriod.OnWeekDaysRepeated :
            daysPeriod.OnWeekDaySingleTime
        );
    }
    if (IsWeekend(weekdays)) {
        return TValue::String(
            repeated ?
            daysPeriod.OnWeekendsRepeated :
            daysPeriod.OnWeekendSingleTime
        );
    }
    if (IsAllWeek(weekdays)) {
        return TValue::String(daysPeriod.EveryDay);
    }

    TVector<TValue> daysText;

    const TWeekday& weekdayMapping = GetWeekdayByLang(lang);

    for (const auto& day : weekdays) {
        daysText.emplace_back(
            Inflect(
                ctx,
                globals,
                TValue::String(weekdayMapping.GetWeekdayName(day)),
                TValue::String(repeated ? "dat,pl" : "acc"),
                /* fio = */ TValue::Bool(false)
            )
        );
    }
    const TPrepcase& preposition = GetPrepCaseByLang(lang);

    TString prep = repeated ?
        preposition.On_WeekDaysRepeated :
        (weekdays.front() == 2 ?
        preposition.On_WeekDaySingleTime2 :
        preposition.On_WeekDaySingleTime
    );

    TText result;
    result.Append(TValue::String(prep + " ").GetString());
    if (daysText.size() > 1) {
        result.Append(Join(ctx, globals, TValue::List({daysText.begin(), std::prev(daysText.end())}), TValue::String(", "), TValue::None()).GetString());
        result.Append(TValue::String(" " + preposition.And + " ").GetString());
        result.Append(daysText.back().GetString());
    } else {
        result.Append(daysText.front().GetString());
    }

    return TValue::String(result);
}

TValue RenderWeekdaySimple(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& weekday) {
    Y_UNUSED(ctx);
    Y_UNUSED(globals);

    TText result;

    if (weekday.IsInteger()) {
        const auto weekdaysValue = weekday.GetInteger();

        // cctz::weekday has values 0 - Monday, 1 - Tuesday, ... 6 - Sunday
        // GetWeekdayName uses 1 - Monday, ... 7 - Sunday

        if (weekdaysValue >= 0 && weekdaysValue <= 6) {
            const TStringBuf lang = ExtractLanguage(ctx);
            result.Append(TValue::String(GetWeekdayByLang(lang).GetWeekdayName(weekdaysValue+1)).GetString());
        }
    }
    return TValue::String(result);
}

TValue RenderDatetimeRaw(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& dt) {
    const TStringBuf lang = ExtractLanguage(ctx);

    const TPrepcase& prepcase = GetPrepCaseByLang(lang);
    const TDatetimeComponents& dtComponents = GetDatetimeRawComponentsByLang(lang);
    const TWeekday& weekdayMapping = GetWeekdayByLang(lang);

    if (dt.GetDict().size() == 3 && GetAttrLoad(dt, "weeks").IsInteger() &&  std::abs(GetAttrLoad(dt, "weeks").GetInteger()) == 1 &&
        dt.GetDict().FindPtr("weeks_relative") && dt.GetDict().FindPtr("weekday")) { // dt contains only "weeks", "weeks_relative", "weekday"
        const auto ordAdj = TValue::String(
            GetAttrLoad(dt, "weeks").GetInteger() == -1 ?
             prepcase.Prev_WeekDay :
             prepcase.Next_WeekDay
            );

        const auto& day = GetAttrLoad(dt, "weekday").GetInteger();
        const auto& weekday = TValue::String(weekdayMapping.GetWeekdayName(day));
        const auto result = Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(prepcase.On_WeekDaySingleTime),
                Inflect(ctx, globals, ordAdj, TValue::String(WEEKDAY_GENDER.at(day)), TValue::Bool(false)),
                Inflect(ctx, globals, weekday, TValue::String("acc"), TValue::Bool(false))
            }),
            TValue::String(" "),
            TValue::None()
        );

        return VoiceAndTextResult(ctx, globals, result, result);
    }

    if (dt.GetDict().size() == 2) {
        if (GetAttrLoad(dt, "seconds_relative").IsBool() && GetAttrLoad(dt, "seconds_relative").GetBool() &&
            GetAttrLoad(dt, "seconds").GetInteger() == 0) {
            return VoiceAndTextResult(ctx, globals, TValue::String(prepcase.Now), TValue::String(prepcase.Now));
        } else if (GetAttrLoad(dt, "minutes_relative").IsBool() && GetAttrLoad(dt, "minutes_relative").GetBool() &&
            GetAttrLoad(dt, "minutes").GetInteger() == 0) {
            return VoiceAndTextResult(ctx, globals, TValue::String(prepcase.Now), TValue::String(prepcase.Now));
        } else if (GetAttrLoad(dt, "hours_relative").IsBool() && GetAttrLoad(dt, "hours_relative").GetBool() &&
            GetAttrLoad(dt, "hours").GetInteger() == 0) {
            return VoiceAndTextResult(ctx, globals, TValue::String(prepcase.Now), TValue::String(prepcase.Now));
        } else if (GetAttrLoad(dt, "days_relative").IsBool() && GetAttrLoad(dt, "days_relative").GetBool()) {
            const TRelativeDay& relativeDays = GetRelativeDayByLang(lang);
            if (GetAttrLoad(dt, "days").GetInteger() == 0) {
                return VoiceAndTextResult(ctx, globals, TValue::String(relativeDays.Today), TValue::String(relativeDays.Today));
            } else if (GetAttrLoad(dt, "days").GetInteger() == 1) {
                return VoiceAndTextResult(ctx, globals, TValue::String(relativeDays.Tomorrow), TValue::String(relativeDays.Tomorrow));
            } else if (GetAttrLoad(dt, "days").GetInteger() == 2) {
                return VoiceAndTextResult(ctx, globals, TValue::String(relativeDays.AfterTomorrow), TValue::String(relativeDays.AfterTomorrow));
            } else if (GetAttrLoad(dt, "days").GetInteger() == -1) {
                return VoiceAndTextResult(ctx, globals, TValue::String(relativeDays.Yesterday), TValue::String(relativeDays.Yesterday));
            } else if (GetAttrLoad(dt, "days").GetInteger() == -2) {
                return VoiceAndTextResult(ctx, globals, TValue::String(relativeDays.BeforeYesterday), TValue::String(relativeDays.BeforeYesterday));
            }
        } else if (GetAttrLoad(dt, "weeks_relative").IsBool() && GetAttrLoad(dt, "weeks_relative").GetBool() &&
            GetAttrLoad(dt, "weeks").GetInteger() == 0) {
            return VoiceAndTextResult(ctx, globals, TValue::String(prepcase.OnThisWeek), TValue::String(prepcase.OnThisWeek));
        } else if (GetAttrLoad(dt, "months_relative").IsBool() && GetAttrLoad(dt, "months_relative").GetBool() &&
            GetAttrLoad(dt, "months").GetInteger() == 0) {
            return VoiceAndTextResult(ctx, globals, TValue::String(prepcase.InThisMonth), TValue::String(prepcase.InThisMonth));
        } else if (GetAttrLoad(dt, "years_relative").IsBool() && GetAttrLoad(dt, "years_relative").GetBool() &&
            GetAttrLoad(dt,"years").GetInteger() == 0) {
            return VoiceAndTextResult(ctx, globals, TValue::String(prepcase.InThisYear), TValue::String(prepcase.InThisYear));
        }
    }

    THashSet<TString> absoluteDateKeys;
    THashSet<TString> absoluteTimeKeys;

    const bool dateRelative = GetAttrLoad(dt, "date_relative").IsBool() && GetAttrLoad(dt, "date_relative").GetBool();
    const bool timeRelative = GetAttrLoad(dt, "time_relative").IsBool() && GetAttrLoad(dt, "time_relative").GetBool();

    TVector<TValue> relativePart;
    static const THashSet<TString> timeKeys = {"hours", "minutes", "seconds"};

    i32 tense = 0;
    i32 currentTense = 0;
    for (const auto& key: {"years", "months", "weeks", "days", "hours", "minutes", "seconds"}) {
        const TStringBuf name = dtComponents.GetDateTimeComponent(key);
        const TValue keyRelattive = GetAttrLoad(dt, JoinSeq("_", {key, "relative"}));
        if ((timeKeys.contains(key) ? timeRelative : dateRelative) || (keyRelattive.IsBool() && keyRelattive.GetBool())) {
            if (const auto quantity = GetAttrLoad(dt, key); quantity.IsInteger()) {
                currentTense = (quantity.GetInteger() > 0 ? 1 : -1);

                if (tense && currentTense != tense) {
                    ythrow yexception() << "Relative parts of the datetime_raw have different signs";
                } else {
                    tense = currentTense;
                }

                const ui32 absQuantity = std::abs(quantity.GetInteger());
                TValue pluralizeQuantity = Pluralize(
                    ctx,
                    globals,
                    TValue::String(name),
                    TValue::Integer(absQuantity),
                    TValue::String("acc")
                );

                TValue phrase = Join(
                    ctx,
                    globals,
                    TValue::List({
                        TValue::String("#acc"),
                        TValue::String(ToString(absQuantity)),
                        pluralizeQuantity
                    }),
                    TValue::String(" "),
                    TValue::None()
                );
                if (lang == NLangs::Ru) {
                    phrase = Replace(ctx, globals, phrase, TValue::String("годов"), TValue::String("лет"));
                }
                relativePart.emplace_back(phrase);
            }
        } else if (timeKeys.contains(key) && dt.GetDict().contains(key)) {
            absoluteTimeKeys.emplace(key);
        } else if (dt.GetDict().contains(key)) {
            absoluteDateKeys.emplace(key);
        }
    }

    TVector<TValue> result;

    if (!relativePart.empty()) {
        if (tense > 0) {
            result.emplace_back(TValue::String(prepcase.After_TimeRelative));
        }

        result.emplace_back(relativePart[0]);
        if (relativePart.size() > 1) {
            for (ui32 ind = 1; ind + 1 < relativePart.size(); ++ind) {
                result.emplace_back(relativePart[ind]);
            }
            result.emplace_back(TValue::String(prepcase.And));
            result.emplace_back(relativePart[relativePart.size() - 1]);
        }

        if (tense < 0) {
            result.emplace_back(TValue::String(prepcase.Backward_TimeRelative));
        }
    }

    const ui32 relPartSize = result.size();

    const auto days = GetAttrLoad(dt, "days");
    const auto months = GetAttrLoad(dt, "months");

    if (absoluteDateKeys.contains("days")) {
        if (days.IsInteger() && days.GetInteger()) {
            if (absoluteDateKeys.contains("months") && months.IsInteger() && months.GetInteger()) {
                const TValue monthInflectName = Inflect(
                    ctx,
                    globals,
                    TValue::String(GetMonthName(months.GetInteger(), lang)),
                    TValue::String("gen"),
                    /* fio = */ TValue::Bool(false)
                );

                result.emplace_back(
                    JoinNumberWithDateTimeComponent(
                        ctx, globals, days.GetInteger(), monthInflectName, "#gen", lang
                    )
                );
            } else {
                result.emplace_back(TValue::String(BuildOnNumberDayPhrase(lang, days.GetInteger())));
            }
        }
    } else if (absoluteDateKeys.contains("months") && months.IsInteger() && months.GetInteger()) {
        const TValue monthInflectName = Inflect(
            ctx,
            globals,
            TValue::String(GetMonthName(months.GetInteger(), lang)),
            TValue::String("abl"),
            /* fio = */ TValue::Bool(false)
        );

        result.emplace_back(Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(prepcase.In_MonthSingleTime),
                monthInflectName
            }),
            TValue::String(" "),
            TValue::None()
        ));
    }

    if (absoluteDateKeys.contains("weeks")) {
        if (GetAttrLoad(dt, "weeks").IsInteger() && GetAttrLoad(dt, "weeks").GetInteger()) {
            ythrow yexception() << "Absolute weeks are currently not supported";
        }
    }
    if (absoluteDateKeys.contains("years")) {
        const auto& yearForms = GetYearFormsByLang(lang);
        const auto years = GetAttrLoad(dt, "years");
        if (years.IsInteger() && years.GetInteger()) {
            if (relPartSize != result.size()) {
                result.emplace_back(TValue::String(JoinSeq(" ", {"#gen", ToString(years.GetInteger()), yearForms.Year_Of})));
            } else {
                result.emplace_back(TValue::String(JoinSeq(" ", {prepcase.In_YearSingleTime, "#loc", ToString(years.GetInteger()), yearForms.Year_In})));
            }
        }
    }

    const auto weekday = GetAttrLoad(dt, "weekday");
    if (weekday.IsInteger() && weekday.GetInteger()) {
        TValue weekdayStr = TValue::String(weekdayMapping.GetWeekdayName(weekday.GetInteger()));

        const TValue weekdayInflectName = Inflect(
            ctx,
            globals,
            weekdayStr,
            TValue::String("acc"),
            /* fio = */ TValue::Bool(false)
        );

        result.emplace_back(Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(weekday.GetInteger() == 2 ? prepcase.On_WeekDaySingleTime2 : prepcase.On_WeekDaySingleTime),
                weekdayInflectName
            }),
            TValue::String(" "),
            TValue::None()
        ));
    }

    if (!absoluteTimeKeys.empty()) {
        if (absoluteTimeKeys.contains("hours")) {
            const auto time = TValue::Dict({
                {NDateKeys::Year, TValue::Integer(1970)},
                {NDateKeys::Month, TValue::Integer(1)},
                {NDateKeys::Day, TValue::Integer(1)},
                {NDateKeys::Hour, GetAttrLoad(dt, "hours") },
                {NDateKeys::Minute, GetAttrLoad(dt, "minutes").IsInteger() ? GetAttrLoad(dt, "minutes") : TValue::Integer(0)},
                {NDateKeys::Second, TValue::Integer(0)},
                {NDateKeys::Microsecond, TValue::Integer(0)},
                {NDateKeys::Timezone, TValue::None()}
            });

            result.emplace_back(TValue::String(JoinSeq(" ", {prepcase.At_TimeAbsolute, ConstructHumanTime(ctx, globals, time)})));
        } else {
            result.emplace_back(TValue::String(prepcase.At_TimeAbsolute));
            if (absoluteTimeKeys.contains("minutes")) {
                const int minutes = GetAttrLoad(dt, "minutes").IsInteger() ? GetAttrLoad(dt, "minutes").GetInteger() : 0;

                const TValue pluralizeMinutes = Pluralize(
                    ctx,
                    globals,
                    TValue::String(dtComponents.Minutes),
                    TValue::Integer(minutes),
                    TValue::String("acc")
                );

                result.emplace_back(
                    JoinNumberWithDateTimeComponent(
                        ctx, globals, minutes, pluralizeMinutes, "#acc", lang
                    )
                );
            }

            if (absoluteTimeKeys.contains("seconds")) {
                const int seconds = GetAttrLoad(dt, "seconds").IsInteger() ? GetAttrLoad(dt, "seconds").GetInteger() : 0;

                const TValue pluralizeSeconds = Pluralize(
                    ctx,
                    globals,
                    TValue::String(dtComponents.Seconds),
                    TValue::Integer(seconds),
                    TValue::String("acc")
                );

                result.emplace_back(
                    JoinNumberWithDateTimeComponent(
                        ctx, globals, seconds, pluralizeSeconds, "#acc", lang
                    )
                );
            }
        }
    }
    auto voiceResult = Join(ctx, globals, TValue::List(result), TValue::String(" "), TValue::None());
    if (lang == NLangs::Ru) {
        for (const auto& [key, val] : DATETIME_REPLACEMENT_PREFIX) {
            if (StrStartsWith(ctx, globals, voiceResult, TValue::String(key)).GetBool()) {
                voiceResult = Replace(ctx, globals, voiceResult, TValue::String(key), TValue::String(val));
                break;
            }
        }
    }

    TValue textResult = voiceResult;

    TVector<TString> possibleTags = {"#acc ", "#gen ", "#loc "};
    for (auto possibleTag : possibleTags) {
        textResult = Replace(ctx, globals, textResult, TValue::String(possibleTag), TValue::String(""));
    }
    return TValue::Dict({
        {"voice", OnlyVoice(ctx, globals, voiceResult)},
        {"text", OnlyText(ctx, globals, textResult)}
    });
}

TValue RenderDateWithOnPreposition(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& dt) {
    if (!dt.IsDict()) {
        ythrow TTypeError() << "dt must be an integer, got a " << dt.GetTypeName();
    }

    const TStringBuf lang = ExtractLanguage(ctx);
    const TPrepcase& prepcase = GetPrepCaseByLang(lang);
    const TYearForms& yearForms = GetYearFormsByLang(lang);

    if (const auto daysRelative = GetAttrLoad(dt, "days_relative"); daysRelative.IsBool() && daysRelative.GetBool()) {
        if (const auto daysDiffValue = GetAttrLoad(dt, "days"); daysDiffValue.IsInteger()) {
            if (const auto daysDiff = daysDiffValue.GetInteger(); daysDiff >= -2 && daysDiff <= 2) {
                const TRelativeDay& relativeDay = GetRelativeDayByLang(lang);
                const auto result = ::Join(' ', prepcase.For_Date, relativeDay.GetRelativeDayName(daysDiff));
                return VoiceAndTextResult(ctx, globals, TValue::String(result), TValue::String(result));
            }
        }
    }

    if (dt.GetDict().size() == 3 &&
        GetAttrLoad(dt, "weeks_relative").IsBool() && GetAttrLoad(dt, "weeks_relative").GetBool() &&
        GetAttrLoad(dt, "weeks").IsInteger() && std::abs(GetAttrLoad(dt, "weeks").GetInteger()) == 1 &&
        GetAttrLoad(dt, "weekday").IsInteger() && GetAttrLoad(dt, "weekday").GetInteger())
    {
        const auto ordAdj = TValue::String(GetAttrLoad(dt, "weeks").GetInteger() == -1 ? prepcase.Prev_WeekDay : prepcase.Next_WeekDay);
        const auto& day = GetAttrLoad(dt, "weekday").GetInteger();
        const auto& weekday = TValue::String(GetWeekdayByLang(lang).GetWeekdayName(day));
        const auto result = Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(prepcase.On_WeekDaySingleTime),
                Inflect(ctx, globals, ordAdj, TValue::String(TString("acc,") + WEEKDAY_GENDER.at(day)), TValue::Bool(false)),
                Inflect(ctx, globals, weekday, TValue::String("acc"), TValue::Bool(false))
            }),
            TValue::String(" "),
            TValue::None()
        );

        return VoiceAndTextResult(ctx, globals, result, result);
    }

    if (GetAttrLoad(dt, "weekday").IsInteger() && GetAttrLoad(dt, "weekday").GetInteger()) {
        const auto& day = GetAttrLoad(dt, "weekday").GetInteger();
        const auto& weekday = TValue::String(GetWeekdayByLang(lang).GetWeekdayName(day));
        const auto result = Join(
            ctx,
            globals,
            TValue::List({
                TValue::String(prepcase.On_WeekDaySingleTime),
                Inflect(ctx, globals, weekday, TValue::String("acc"), TValue::Bool(false))
            }),
            TValue::String(" "),
            TValue::None()
        );

        return VoiceAndTextResult(ctx, globals, result, result);
    }

    const bool yearExists = dt.GetDict().contains("years");
    const bool monthExists = dt.GetDict().contains("months");

    const auto nowYear = NDatetime::ToCivilTime(TInstant::Now(), NDatetime::GetUtcTimeZone()).RealYear();
    const auto dateYear = dt.GetDict().Value("years", TValue::Integer(nowYear)).GetInteger();

    const auto date = TValue::Dict({
        {NDateKeys::Year,TValue::Integer(dateYear)},
        {NDateKeys::Month, dt.GetDict().Value("months", TValue::Integer(1))},
        {NDateKeys::Day, dt.GetDict().Value("days", TValue::Integer(1))},
        {NDateKeys::Hour, TValue::Integer(0)},
        {NDateKeys::Minute, TValue::Integer(0)},
        {NDateKeys::Second, TValue::Integer(0)},
        {NDateKeys::Microsecond, TValue::Integer(0)},
        {NDateKeys::Timezone, TValue::None()}
    });

    auto parts = TValue::List({TValue::String(prepcase.For_Date)});

    if (dt.GetDict().contains("days")) {
        const auto day = GetAttrLoad(dt, "days").GetInteger();
        parts.GetMutableList().emplace_back(TValue::String(ToString(day)));
        parts.GetMutableList().emplace_back(HumanMonth(ctx, globals, date, TValue::String("gen,n")));
    } else if (monthExists) {
        parts.GetMutableList().emplace_back(HumanMonth(ctx, globals, date, TValue::String("acc,n")));
    }

    if (dateYear != nowYear || (yearExists && !monthExists)) {
        parts.GetMutableList().emplace_back(TValue::String(ToString(dateYear)));
        parts.GetMutableList().emplace_back(TValue::String(monthExists ? yearForms.Year_Of : GetDatetimeRawComponentsByLang(lang).Years));
    }

    return Join(ctx, globals, parts, TValue::String(" "), TValue::None());
}

TValue RenderUnitsTime(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& units, const TValue& cases) {
    const TStringBuf lang = ExtractLanguage(ctx);
    const TDatetimeComponents& dtComponents = GetDatetimeRawComponentsByLang(lang);

    const TVector<std::pair<TStringBuf, TStringBuf>> unitsTimeRawComponents = {
        {"hours", dtComponents.Hours},
        {"minutes", dtComponents.Minutes},
        {"seconds", dtComponents.Seconds},
    };

    const TString casesStr = cases.GetString().GetStr();

    int hours = 0;
    if (auto hoursVal = GetAttrLoad(units, "hours"); hoursVal.IsInteger()) {
        hours = hoursVal.GetInteger();
    }

    int minutes = 0;
    if (auto minutesVal = GetAttrLoad(units, "minutes"); minutesVal.IsInteger()) {
        minutes = minutesVal.GetInteger();
    }

    int seconds;
    if (auto secondsVal = GetAttrLoad(units, "seconds"); secondsVal.IsInteger()) {
        seconds = secondsVal.GetInteger();
    }

    TVector<TValue> result;
    for (auto [component, word] : unitsTimeRawComponents) {
        int value = 0;
        if (auto valueVal = GetAttrLoad(units, component); valueVal.IsInteger()) {
            value = valueVal.GetInteger();
        }

        if (value) {
            TValue pluralizeValue = Pluralize(
                ctx,
                globals,
                TValue::String(word),
                TValue::Integer(value),
                cases
            );

            result.emplace_back(Join(
                ctx,
                globals,
                TValue::List({
                    TValue::String(JoinSeq("", {"#", casesStr})),
                    TValue::String(ToString(value)),
                    pluralizeValue
                }),
                TValue::String(" "),
                TValue::None()
            ));
        }
    }

    TValue voiceResult = Join(ctx, globals, TValue::List(result), TValue::String(" "), TValue::None());
    TValue textResult = voiceResult;

    TVector<TString> possibleTags = {"#acc ", "#gen ", "#loc ", "#nom "};
    for (auto possibleTag : possibleTags) {
        textResult = Replace(ctx, globals, textResult, TValue::String(possibleTag), TValue::String(""));
    }

    return TValue::Dict({
        {"voice", OnlyVoice(ctx, globals, voiceResult)},
        {"text", OnlyText(ctx, globals, textResult)}
    });
}

TValue TimeFormat(const TCallCtx& ctx, const TGlobalsChain* globals, const TValue& time, const TValue& cases) {
    const TString casesStr = cases.GetString().GetStr();

    const TStringBuf lang = ExtractLanguage(ctx);

    const TDatetimeComponents& dtComponents = GetDatetimeRawComponentsByLang(lang);
    const TDayPartNameForTime dayPartName = GetDayPartNameForTimeByLang(lang);

    int hours = 0;
    if (auto hoursVal = GetAttrLoad(time, "hours"); hoursVal.IsInteger()) {
        hours = hoursVal.GetInteger();
    }

    int minutes = 0;
    if (auto minutesVal = GetAttrLoad(time, "minutes"); minutesVal.IsInteger()) {
        minutes = minutesVal.GetInteger();
    }

    TString period;
    if (auto periodVal = GetAttrLoad(time, "period"); periodVal.IsString()) {
        period = periodVal.GetString().GetStr();
    }

    if (period == "pm" && hours != 12) {
        hours += 12;
    }

    if (period == "am" && hours == 12) {
        hours = 0;
    }

    TValue voiceResult;
    TValue textResult;

    const auto skCase = CASES_FOR_SK.at(casesStr);
    const TValue hoursStr = TValue::String(dtComponents.Hours);
    const TValue minutesStr = TValue::String(dtComponents.Minutes);
    const TValue nightsStr = TValue::String(dayPartName.OfNight);

    if (minutes == 0) {
        if (hours == 0) {
            const TValue pluralizeHours = Pluralize(
                ctx,
                globals,
                hoursStr,
                TValue::Integer(12),
                cases
            );

            textResult = Join(
                ctx,
                globals,
                TValue::List({
                    TValue::String("12"),
                    pluralizeHours,
                    nightsStr
                }),
                TValue::String(" "),
                TValue::None()
            );
            voiceResult = Join(
                ctx,
                globals,
                TValue::List({
                    TValue::String(JoinSeq("", {"#", skCase})),
                    textResult
                }),
                TValue::String(" "),
                TValue::None()
            );
        } else if (hours == 1) {
            const TValue inflectHours = Inflect(
                ctx,
                globals,
                hoursStr,
                cases,
                /* fio = */ TValue::Bool(false)
            );

            textResult = TValue::String(JoinSeq(" ", {inflectHours.GetString().GetStr(), nightsStr.GetString().GetStr()}));
            voiceResult = textResult;
        } else {
            const TValue pluralizeHours = Pluralize(
                ctx,
                globals,
                hoursStr,
                TValue::Integer(hours),
                cases
            );

            TString textResultSuffix;
            if (hours == 2 || hours == 3) {
                textResultSuffix = " " + dayPartName.OfNight;
            } else if (hours < 12) {
                textResultSuffix = " " + dayPartName.OfMorning;
            } else if (hours == 12) {
                textResultSuffix = " " + dayPartName.OfAfternoon;
            }

            textResult = TValue::String(TString::Join(ToString(hours), " ", pluralizeHours.GetString().GetStr(), textResultSuffix));
            voiceResult = Join(
                ctx,
                globals,
                TValue::List({
                    TValue::String(TString::Join("#", skCase)),
                    textResult
                }),
                TValue::String(" "),
                TValue::None()
            );
        }
    } else {
        const TValue pluralizeHours = Pluralize(
            ctx,
            globals,
            hoursStr,
            TValue::Integer(hours),
            cases
        );

        const TValue pluralizeMinutes = Pluralize(
            ctx,
            globals,
            minutesStr,
            TValue::Integer(minutes),
            cases
        );

        voiceResult = Join(
            ctx,
            globals,
            TValue::List({
                 TValue::String(JoinSeq("", {"#", casesStr})),
                 TValue::String(ToString(hours)),
                 pluralizeHours,
                 TValue::String(JoinSeq("", {"#", casesStr})),
                 TValue::String(ToString(minutes)),
                 pluralizeMinutes
            }),
            TValue::String(" "),
            TValue::None()
        );

        textResult = TValue::String(JoinSeq("", {(hours < 10 ? "0" : ""), ToString(hours), ":", (minutes < 10 ? "0" : ""), ToString(minutes)}));
    }

    return TValue::Dict({
        {"voice", OnlyVoice(ctx, globals, voiceResult)},
        {"text", OnlyText(ctx, globals, textResult)}
    });
}

void Macro_ChooseLine(const TCallCtx& ctx, const TCaller* caller, const TGlobalsChain*, IOutputStream& out) {
    Y_ASSERT(caller);

    ctx.CallStack.push_back({"<choose line>", {}, {}});

    // XXX(a-square): the way we split and strip the lines text is technically wrong,
    // because we ignore flags while doing it, but amazingly it is consistent with
    // how the Python backend does it so we keep compatibility.
    //
    // ATM I don't believe there is a better way than this to do chooseline.
    TText lines;
    TTextOutput linesOut(lines);
    InvokeCaller(caller, linesOut);

    auto bounds = lines.GetBounds();

    TVector<TStringBuf> splitLines;
    for (auto it : StringSplitter(bounds).Split('\n')) {
        // if a line is split inside a text or a voice tag, the result will probably be nonsensical,
        // e.g. {%vc%}foo\nbar{%evc%}{%tx%}baz{%etx%} will be treated as two choices:
        // 1. text = "", voice = "foo"
        // 2. text = "baz", voice = "bar"
        //
        // Since this probably not the intended result, we check and throw if that's the case.
        // The Python backend would've also split the line here, resulting in choices with
        // unbalanced tags, causing post-processing to die.
        size_t delimOffset = it.TokenDelim() - bounds.begin();
        if (delimOffset < bounds.size() && lines.GetFlagsAt(delimOffset) != TText::AllFlags()) {
            ythrow TValueError() << "Line split inside a text/voice tag";
        }

        splitLines.push_back(it.Token());
    }

    TVector<TStringBuf> strippedLines;
    strippedLines.reserve(splitLines.size());
    for (auto line : splitLines) {
        auto strippedLine = StripString(line);
        if (strippedLine) {
            strippedLines.push_back(strippedLine);
        }
    }

    if (!strippedLines.empty()) {
        // clip the text to the chosen line and output
        out << lines.GetView(strippedLines[ctx.Rng.RandomInteger(strippedLines.size())]);
    }

    ctx.CallStack.pop_back();
}

} // namespace NAlice::NNlg::NBuiltins
