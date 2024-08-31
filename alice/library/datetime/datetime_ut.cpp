// TODO
// * на переход с одного дня на другой
// * на переход с летнего на зимнее время (сейчас работать не будет т.к. надо отказываться от TSimpleTM
// * указать большой интеравал - он должен сократиться
// * если указана неверное число - вернуть ошибку (i.e. 32 февраля)
// * add check for invalid daypartname
#include "datetime.h"

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/scheme/scheme.h>
#include <util/generic/vector.h>

namespace {

const TStringBuf TZ_NAME = "Etc/GMT-4";

const NDatetime::TTimeZone& Tz() {
    static const NDatetime::TTimeZone tz = NDatetime::GetTimeZone(TString(TZ_NAME));
    return tz;
}

// from sunday (0) to saturday (6)
const TVector<std::pair<time_t, NDatetime::TSimpleTM>>& Epochs() {
    static TVector<std::pair<time_t, NDatetime::TSimpleTM>> epochs;
    if (!epochs) {
        time_t epoch = 1486286753; // Sun Feb  5 12:25:53 MSK 2017
        for (ssize_t i = 7; i > 0; --i, epoch += 86400) {
            epochs.push_back({ epoch, NDatetime::ToCivilTime(TInstant::Seconds(epoch), Tz()) });
        }
    }

    return epochs;
}

}

using namespace NAlice;

// BASS-Like slot
struct TJsonSlot {
    TJsonSlot(TStringBuf name, TStringBuf type)
        : Name(name)
        , Type(type)
    {}

    TString Name;
    TString Type;
    NSc::TValue Value;
};

// Hollywood-Like slot
struct TStringSlot {
    TStringSlot(TStringBuf name, TStringBuf type)
        : Name(name)
        , Type(type)
    {}

    TString Name;
    TString Type;
    TString Value;
};

template<typename TSlotType>
struct TOneMock {
    struct TYMDhms {
        ui32 Year;
        ui32 Month;
        ui32 Day;
        ui32 Hour;
        ui32 Min;
        ui32 Sec;
        NDatetime::TSimpleTM ToTM(const NDatetime::TTimeZone& tz) const {
            return NDatetime::CreateCivilTime(tz, Year, Month, Day, Hour, Min, Sec);
        }
    };
    TOneMock(TStringBuf dtJson, time_t uEpoch, TDateTime::EDayPart dp, TYMDhms&& ymdhms)
        : Str(dtJson)
        , UserEpoch(uEpoch)
        , ReferenceDT(ymdhms)
        , ReferenceDayPart(dp)
        , DateTimeSlot(new TSlotType("when", "datetime_raw"))
    {
        if constexpr (std::is_same_v<TStringSlot, TSlotType>) {
            DateTimeSlot->Value = dtJson;
        } else {
            DateTimeSlot->Value = NSc::TValue::FromJson(dtJson);
        }
    }

    TOneMock(TStringBuf dtJson, TStringBuf dpJson, time_t uEpoch, TDateTime::EDayPart dp, TYMDhms&& ymdhms)
        : Str(dtJson)
        , UserEpoch(uEpoch)
        , ReferenceDT(ymdhms)
        , ReferenceDayPart(dp)
        , DateTimeSlot(new TSlotType("when", "datetime_raw"))
        , DayPartSlot(new TSlotType("day_part", "day_part"))
    {
        if constexpr (std::is_same_v<TStringSlot, TSlotType>) {
            DateTimeSlot->Value = dtJson;
            DayPartSlot->Value = dpJson;
        } else {
            DateTimeSlot->Value = NSc::TValue::FromJson(dtJson);
            DayPartSlot->Value = NSc::TValue::FromJson(dpJson);
        }
    }

    const TStringBuf Str;
    const time_t UserEpoch;
    const TYMDhms ReferenceDT;
    const TDateTime::EDayPart ReferenceDayPart;
    std::unique_ptr<TSlotType> DateTimeSlot;
    std::unique_ptr<TSlotType> DayPartSlot;
};

template<typename TSlotType>
class TOneDayMocker {
public:
    TOneDayMocker(TStringBuf tzName, bool lookForward)
        : TZ(NDatetime::GetTimeZone(TString{tzName}))
        , LookForward(lookForward)
    {
    }

    TOneDayMocker& operator <<(TOneMock<TSlotType>&& m) {
        Mocks.emplace_back(std::move(m));
        return *this;
    }

    void TestIt() {
        for (auto& mock : Mocks) {
            std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot(
                mock.DateTimeSlot.get(), mock.DayPartSlot.get(),
                TDateTime(TDateTime::TSplitTime(TZ, mock.UserEpoch)),
                { 10, LookForward }
            );

            TStringBuilder name;
            name << mock.Str << " : " << mock.ReferenceDayPart;

            UNIT_ASSERT(dtr);
            UNIT_ASSERT_C(!dtr->IsNow(), name.data());
            UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 1);
            auto it = dtr->cbegin();
            UNIT_ASSERT(it != dtr->cend());

            name << " : " << it->SplitTime().ToString() << " == (ref) " << mock.ReferenceDT.ToTM(TZ).ToString();
            name << mock.Str << " : " << mock.ReferenceDayPart << " : " << it->SplitTime().ToString() << " == (ref) " << mock.ReferenceDT.ToTM(TZ).ToString();
            UNIT_ASSERT_EQUAL_C(it->DayPart(), mock.ReferenceDayPart, it->DayPartAsString().data());
            static constexpr TStringBuf format = "%Y%m%d%H%M%S";
            UNIT_ASSERT_EQUAL_C(it->SplitTime().ToString(format.data()), mock.ReferenceDT.ToTM(TZ).ToString(format.data()), name.data());
        }
    }

private:
    const NDatetime::TTimeZone TZ;
    const bool LookForward;
    TVector<TOneMock<TSlotType>> Mocks;
};

Y_UNIT_TEST_SUITE(BassDateTime) {
    Y_UNIT_TEST(Weekday) {
        TJsonSlot jsonSlot("when", "datetime");
        TStringSlot stringSlot("when", "datetime");
        std::unique_ptr<TDateTimeList> dtl;

        {
            jsonSlot.Value = NSc::TValue::FromJson(R"({ "weeks": -1, "weeks_relative": true, "weekday": 1 })");
            dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
                    &jsonSlot, nullptr,
                    // Wed Mar  1 13:54:51 MSK 2017
                    TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                    { 1, false }
                    );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Mon, 20 Feb 2017 13:54:51 +0300", "Prev Mon (no lookahead)");
        }
        {
            stringSlot.Value = R"({ "weeks": -1, "weeks_relative": true, "weekday": 1 })";
            dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
                    &stringSlot, nullptr,
                    // Wed Mar  1 13:54:51 MSK 2017
                    TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                    { 1, false }
                    );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Mon, 20 Feb 2017 13:54:51 +0300", "Prev Mon (no lookahead)");
        }

        {
            jsonSlot.Value = NSc::TValue::FromJson(R"({ "weekday": 2 })");
            dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
                &jsonSlot, nullptr,
                // Wed Mar  1 13:54:51 MSK 2017
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, true }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Tue, 07 Mar 2017 13:54:51 +0300", "Next Thu (lookahead)");

            dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
                &jsonSlot, nullptr,
                // Wed Mar  1 13:54:51 MSK 2017
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, false }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Tue, 28 Feb 2017 13:54:51 +0300", "Past Thu (no lookahead)");
        }
        {
            stringSlot.Value = R"({ "weekday": 2 })";
            dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
                &stringSlot, nullptr,
                // Wed Mar  1 13:54:51 MSK 2017
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, true }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Tue, 07 Mar 2017 13:54:51 +0300", "Next Thu (lookahead)");

            dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
                &stringSlot, nullptr,
                // Wed Mar  1 13:54:51 MSK 2017
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, false }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Tue, 28 Feb 2017 13:54:51 +0300", "Past Thu (no lookahead)");
        }

        {
            jsonSlot.Value = NSc::TValue::FromJson(R"({ "weeks": 1, "weeks_relative": true, "weekday": 1 })");
            dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
                &jsonSlot, nullptr,
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, true }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Mon, 06 Mar 2017 13:54:51 +0300", "Next week Mon (look ahead)");

            dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
                &jsonSlot, nullptr,
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, false }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Mon, 06 Mar 2017 13:54:51 +0300", "Next week Mon (no lookahead)");
        }
        {
            stringSlot.Value = R"({ "weeks": 1, "weeks_relative": true, "weekday": 1 })";
            dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
                &stringSlot, nullptr,
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, true }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Mon, 06 Mar 2017 13:54:51 +0300", "Next week Mon (look ahead)");

            dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
                &stringSlot, nullptr,
                TDateTime(TDateTime::TSplitTime(NDatetime::GetTimeZone("Europe/Moscow"), 1488365691)),
                { 1, false }
            );
            UNIT_ASSERT(dtl);
            UNIT_ASSERT_EQUAL_C(dtl->cbegin()->SplitTime().ToString(), "Mon, 06 Mar 2017 13:54:51 +0300", "Next week Mon (no lookahead)");
        }
    }

    Y_UNIT_TEST(CurrentJson) {
        TVector<std::unique_ptr<TJsonSlot>> slots;
        slots.push_back(std::make_unique<TJsonSlot>("when", "datetime"));
        slots.push_back(std::make_unique<TJsonSlot>("when", "datetime"));
        slots.back()->Value.SetDict();

        slots.push_back(std::make_unique<TJsonSlot>("when", "datetime"));
        // it means "now"! https://st.yandex-team.ru/ASSISTANT-6
        slots.back()->Value = NSc::TValue::FromJson(R"({ "seconds": 0, "seconds_relative": true })");

        for (const auto& slot : slots) {
            {
                std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TJsonSlot>(
                    slot.get(), nullptr,
                    TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
                    { 10, false }
                );
                UNIT_ASSERT(dtr);
                UNIT_ASSERT(dtr->IsNow());
                UNIT_ASSERT(!dtr->TotalDays());
            }

            {
                TJsonSlot dayPart("day_part", "string");
                dayPart.Value.SetNull();
                std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot(
                    slot.get(), &dayPart,
                    TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
                    { 10, false }
                );
                UNIT_ASSERT(dtr);
                UNIT_ASSERT(dtr->IsNow());
                UNIT_ASSERT(!dtr->TotalDays());
            }

            { // if slot day_part is set, we don't think that the requested weather could be now!
                TJsonSlot dayPart("day_part", "string");
                dayPart.Value.SetString("night");
                std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot(
                    slot.get(), &dayPart,
                    TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
                    { 10, false }
                );
                UNIT_ASSERT(dtr);
                UNIT_ASSERT(!dtr->IsNow());
                UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 1);
                UNIT_ASSERT_VALUES_EQUAL(dtr->cbegin()->DayPart(), TDateTime::EDayPart::Night);
            }
        }
    }

    Y_UNIT_TEST(CurrentString) {
        TVector<std::unique_ptr<TStringSlot>> slots;
        slots.push_back(std::make_unique<TStringSlot>("when", "datetime"));
        slots.push_back(std::make_unique<TStringSlot>("when", "datetime"));
        slots.back()->Value = "{}";

        slots.push_back(std::make_unique<TStringSlot>("when", "datetime"));
        // it means "now"! https://st.yandex-team.ru/ASSISTANT-6
        slots.back()->Value = R"({ "seconds": 0, "seconds_relative": true })";

        for (const auto& slot : slots) {
            {
                std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TStringSlot>(
                    slot.get(), nullptr,
                    TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
                    { 10, false }
                );
                UNIT_ASSERT(dtr);
                UNIT_ASSERT(dtr->IsNow());
                UNIT_ASSERT(!dtr->TotalDays());
            }

            {
                TStringSlot dayPart("day_part", "string");
                dayPart.Value = "null";
                std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot(
                    slot.get(), &dayPart,
                    TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
                    { 10, false }
                );
                UNIT_ASSERT(dtr);
                UNIT_ASSERT(dtr->IsNow());
                UNIT_ASSERT(!dtr->TotalDays());
            }

            { // if slot day_part is set, we don't think that the requested weather could be now!
                TStringSlot dayPart("day_part", "string");
                dayPart.Value = "night";
                std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot(
                    slot.get(), &dayPart,
                    TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
                    { 10, false }
                );
                UNIT_ASSERT(dtr);
                UNIT_ASSERT(!dtr->IsNow());
                UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 1);
                UNIT_ASSERT_VALUES_EQUAL(dtr->cbegin()->DayPart(), TDateTime::EDayPart::Night);
            }
        }
    }

    Y_UNIT_TEST(TodayWithLookForwardJson) {
        TOneDayMocker<TJsonSlot> m("Australia/Sydney", true);
        // Sun Feb  5 20:25:53 AEDT 2017
        time_t e = 1486286753;
        // сегодня (время не меняем, т.к. wholeday по дефолту)
        m << TOneMock<TJsonSlot>(R"({ "days": 0, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 20, 25, 53 });
        // сегодня утром (время устанавливаем в 6мь утра, т.к. по нашему утро начинается в 6
        m << TOneMock<TJsonSlot>(R"({ "days": 0, "days_relative": true })", "morning", e, TDateTime::EDayPart::Morning, { 2017, 2, 5, 6, 0, 0 });
        // сегодня в 15 (обнуляем минуты, секунды)
        m << TOneMock<TJsonSlot>(R"({ "days": 0, "days_relative": true, "hours": 15 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 15, 0, 0 });
        // утром (т.к. сейчас вечер добавляем +1 день (т.к. WithLookForward), и ставим утро)
        m << TOneMock<TJsonSlot>("null", "morning", e, TDateTime::EDayPart::Morning, { 2017, 2, 6, 6, 0, 0 });
        // FIXME мы пока не поддерживаем в 8мь вечера, но надо сделать и тогда заэнейблить этот тест
        //m << TOneMock(R"({ "days": 0, "days_relative": true, "hours": 8 })", "evening", 1486286753, TDateTime::EDayPart::Day, { 2017, 2, 5, 15, 0, 0 });
        m.TestIt();
    }

    Y_UNIT_TEST(TodayWithLookForwardString) {
        TOneDayMocker<TStringSlot> m("Australia/Sydney", true);
        // Sun Feb  5 20:25:53 AEDT 2017
        time_t e = 1486286753;
        // сегодня (время не меняем, т.к. wholeday по дефолту)
        m << TOneMock<TStringSlot>(R"({ "days": 0, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 20, 25, 53 });
        // сегодня утром (время устанавливаем в 6мь утра, т.к. по нашему утро начинается в 6
        m << TOneMock<TStringSlot>(R"({ "days": 0, "days_relative": true })", "morning", e, TDateTime::EDayPart::Morning, { 2017, 2, 5, 6, 0, 0 });
        // сегодня в 15 (обнуляем минуты, секунды)
        m << TOneMock<TStringSlot>(R"({ "days": 0, "days_relative": true, "hours": 15 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 15, 0, 0 });
        // утром (т.к. сейчас вечер добавляем +1 день (т.к. WithLookForward), и ставим утро)
        m << TOneMock<TStringSlot>("null", "morning", e, TDateTime::EDayPart::Morning, { 2017, 2, 6, 6, 0, 0 });
        // FIXME мы пока не поддерживаем в 8мь вечера, но надо сделать и тогда заэнейблить этот тест
        //m << TOneMock(R"({ "days": 0, "days_relative": true, "hours": 8 })", "evening", 1486286753, TDateTime::EDayPart::Day, { 2017, 2, 5, 15, 0, 0 });
        m.TestIt();
    }

    Y_UNIT_TEST(LookForwardFutureJson) {
        TOneDayMocker<TJsonSlot> m("Europe/Moscow", true);

        // Tue Dec 25 21:26:57 MSK 2018
        time_t e = 1545762417;

        m << TOneMock<TJsonSlot>(R"({ "days": 1 })", e, TDateTime::EDayPart::WholeDay, { 2019, 1, 1, 0, 0, 0 });
        m << TOneMock<TJsonSlot>(R"({ "days": 1, "months": 1 })", e, TDateTime::EDayPart::WholeDay, { 2019, 1, 1, 0, 0, 0 });
        m << TOneMock<TJsonSlot>(R"({ "days": 5, "months": 2 })", e, TDateTime::EDayPart::WholeDay, { 2019, 2, 5, 0, 0, 0 });
        m.TestIt();
    }

    Y_UNIT_TEST(LookForwardFutureString) {
        TOneDayMocker<TStringSlot> m("Europe/Moscow", true);

        // Tue Dec 25 21:26:57 MSK 2018
        time_t e = 1545762417;

        m << TOneMock<TStringSlot>(R"({ "days": 1 })", e, TDateTime::EDayPart::WholeDay, { 2019, 1, 1, 0, 0, 0 });
        m << TOneMock<TStringSlot>(R"({ "days": 1, "months": 1 })", e, TDateTime::EDayPart::WholeDay, { 2019, 1, 1, 0, 0, 0 });
        m << TOneMock<TStringSlot>(R"({ "days": 5, "months": 2 })", e, TDateTime::EDayPart::WholeDay, { 2019, 2, 5, 0, 0, 0 });
        m.TestIt();
    }

    Y_UNIT_TEST(OneSpecificDayWithLookForwardJson) {
        TOneDayMocker<TJsonSlot> m("Europe/Kiev", true);

        // Sun Feb  5 11:25:53 EET 2017
        time_t e = 1486286753;

        // завтра
        m << TOneMock<TJsonSlot>(R"({ "days": 1, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 6, 11, 25, 53 });
        // послезавтра
        m << TOneMock<TJsonSlot>(R"({ "days": 2, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 7, 11, 25, 53 });
        // второго (имеется ввиду второго числа в будущем, т.к. lookforward)  (2 feb 2017)
        m << TOneMock<TJsonSlot>(R"({ "days": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 3, 2, 0, 0, 0 });
        // через неделю (в этот же день) (2017 Feb 12 12:25:53)
        m << TOneMock<TJsonSlot>(R"({ "weeks": 1, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 12, 11, 25, 53 });
        // на этой неделе (это уже datetime_range, но нам важно что бы просто выбрался сегодняшний день без модификаций)
        m << TOneMock<TJsonSlot>(R"({ "weeks": 0, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // 10 мая 2016 (обнулим время)
        m << TOneMock<TJsonSlot>(R"({ "years": 2016, "months": 5, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2016, 5, 10, 0, 0, 0 });
        // 10 мая 2016 в 12:30
        m << TOneMock<TJsonSlot>(R"({ "years": 2016, "months": 5, "days": 10, "hours": 12, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2016, 5, 10, 12, 30, 0 });
        // 20 марта 2016 в 19 часов
        m << TOneMock<TJsonSlot>(R"({ "years": 2016, "months": 3, "days": 20, "hours": 19 })", e, TDateTime::EDayPart::Evening, { 2016, 3, 20, 19, 0, 0 });
        // 10го числа следующего месяца (Tue May 10 00:00:00 MSK 2016)
        m << TOneMock<TJsonSlot>(R"({ "months": 1, "months_relative": true, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2017, 3, 10, 0, 0, 0 });
        // в 16:30 (Sun Feb  5 16:30:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "hours": 16, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 16, 30, 0 });
        // через два часа (Sun Feb  5 13:00:00 2017)
        m << TOneMock<TJsonSlot>(R"({ "hours": 2, "hours_relative": true })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 13, 25, 53 });
        // через 28 минут (Sun Feb  5 11:53:53 2017)
        m << TOneMock<TJsonSlot>(R"({ "minutes": 28, "minutes_relative": true })", e, TDateTime::EDayPart::Morning, { 2017, 2, 5, 11, 53, 53 });
        // в понедельник вечером (Mon Feb  6 18:00:00 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 1 })", "evening", e, TDateTime::EDayPart::Evening, { 2017, 2, 6, 18, 0, 0 });
        // в понедельник (Mon Feb  6 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 1 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 6, 11, 25, 53 });
        // во вторник (Tue Feb  7 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 7, 11, 25, 53 });
        // в среду (Wed Feb  8 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 3 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 8, 11, 25, 53 });
        // в четверг (Thu Feb  9 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 4 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 9, 11, 25, 53 });
        // в пятницу (Fri Feb 10 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 5 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 10, 11, 25, 53 });
        // в субботу (Sat Feb 11 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 6 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 11, 11, 25, 53 });
        // в субботу в полночь (Sat Feb 11 00:00:00 MSK 2017) FIXME actually it is incorrect, must be Feb 12
        m << TOneMock<TJsonSlot>(R"({ "weekday": 6, "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 11, 0, 0, 0 });
        // в воскресенье; и т.к. сегодня воскресенье - возвращаем сегодня! (Sun Feb  5 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 7 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // Вчера в пятнадцать минут первого ночи
        m << TOneMock<TJsonSlot>(R"({ "days": -1, "days_relative": true, "hours": 0, "minutes": 15 })", e, TDateTime::EDayPart::Night, { 2017, 2, 4, 0, 15, 0 });
        // через 3 часа, 9 минут, 15 секунд
        m << TOneMock<TJsonSlot>(R"({ "time_relative": true, "hours": 3, "minutes": 9, "seconds": 15 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 14, 35, 8 });
        // в полночь (прибавляем один день)
        m << TOneMock<TJsonSlot>(R"({ "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 6, 0, 0, 0 });
        m << TOneMock<TJsonSlot>("null", "night", e, TDateTime::EDayPart::Night, { 2017, 2, 6, 0, 0, 0 });
        // Через год и три с половиной месяца (XXX в этом тесте ломается таймзона, т.к. TSimpleTM не правильно делает Add(), поэтому сравниваем все время без таймзоны)
        m << TOneMock<TJsonSlot>(R"({ "months": 3, "years": 1, "weeks": 2, "months_relative": true, "years_relative": true, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2018, 5, 19, 11, 25, 53 });

        // XXX не работает, надо отдавать ошибку на 32 число
        //m << TOneMock((R"({ "days": 32 })", cepoch, 1488273953);
        m.TestIt();
    }

    Y_UNIT_TEST(OneSpecificDayWithLookForwardString) {
        TOneDayMocker<TStringSlot> m("Europe/Kiev", true);

        // Sun Feb  5 11:25:53 EET 2017
        time_t e = 1486286753;

        // завтра
        m << TOneMock<TStringSlot>(R"({ "days": 1, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 6, 11, 25, 53 });
        // послезавтра
        m << TOneMock<TStringSlot>(R"({ "days": 2, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 7, 11, 25, 53 });
        // второго (имеется ввиду второго числа в будущем, т.к. lookforward)  (2 feb 2017)
        m << TOneMock<TStringSlot>(R"({ "days": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 3, 2, 0, 0, 0 });
        // через неделю (в этот же день) (2017 Feb 12 12:25:53)
        m << TOneMock<TStringSlot>(R"({ "weeks": 1, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 12, 11, 25, 53 });
        // на этой неделе (это уже datetime_range, но нам важно что бы просто выбрался сегодняшний день без модификаций)
        m << TOneMock<TStringSlot>(R"({ "weeks": 0, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // 10 мая 2016 (обнулим время)
        m << TOneMock<TStringSlot>(R"({ "years": 2016, "months": 5, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2016, 5, 10, 0, 0, 0 });
        // 10 мая 2016 в 12:30
        m << TOneMock<TStringSlot>(R"({ "years": 2016, "months": 5, "days": 10, "hours": 12, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2016, 5, 10, 12, 30, 0 });
        // 20 марта 2016 в 19 часов
        m << TOneMock<TStringSlot>(R"({ "years": 2016, "months": 3, "days": 20, "hours": 19 })", e, TDateTime::EDayPart::Evening, { 2016, 3, 20, 19, 0, 0 });
        // 10го числа следующего месяца (Tue May 10 00:00:00 MSK 2016)
        m << TOneMock<TStringSlot>(R"({ "months": 1, "months_relative": true, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2017, 3, 10, 0, 0, 0 });
        // в 16:30 (Sun Feb  5 16:30:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "hours": 16, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 16, 30, 0 });
        // через два часа (Sun Feb  5 13:00:00 2017)
        m << TOneMock<TStringSlot>(R"({ "hours": 2, "hours_relative": true })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 13, 25, 53 });
        // через 28 минут (Sun Feb  5 11:53:53 2017)
        m << TOneMock<TStringSlot>(R"({ "minutes": 28, "minutes_relative": true })", e, TDateTime::EDayPart::Morning, { 2017, 2, 5, 11, 53, 53 });
        // в понедельник вечером (Mon Feb  6 18:00:00 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 1 })", "evening", e, TDateTime::EDayPart::Evening, { 2017, 2, 6, 18, 0, 0 });
        // в понедельник (Mon Feb  6 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 1 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 6, 11, 25, 53 });
        // во вторник (Tue Feb  7 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 7, 11, 25, 53 });
        // в среду (Wed Feb  8 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 3 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 8, 11, 25, 53 });
        // в четверг (Thu Feb  9 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 4 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 9, 11, 25, 53 });
        // в пятницу (Fri Feb 10 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 5 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 10, 11, 25, 53 });
        // в субботу (Sat Feb 11 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 6 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 11, 11, 25, 53 });
        // в субботу в полночь (Sat Feb 11 00:00:00 MSK 2017) FIXME actually it is incorrect, must be Feb 12
        m << TOneMock<TStringSlot>(R"({ "weekday": 6, "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 11, 0, 0, 0 });
        // в воскресенье; и т.к. сегодня воскресенье - возвращаем сегодня! (Sun Feb  5 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 7 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // Вчера в пятнадцать минут первого ночи
        m << TOneMock<TStringSlot>(R"({ "days": -1, "days_relative": true, "hours": 0, "minutes": 15 })", e, TDateTime::EDayPart::Night, { 2017, 2, 4, 0, 15, 0 });
        // через 3 часа, 9 минут, 15 секунд
        m << TOneMock<TStringSlot>(R"({ "time_relative": true, "hours": 3, "minutes": 9, "seconds": 15 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 14, 35, 8 });
        // в полночь (прибавляем один день)
        m << TOneMock<TStringSlot>(R"({ "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 6, 0, 0, 0 });
        m << TOneMock<TStringSlot>("null", "night", e, TDateTime::EDayPart::Night, { 2017, 2, 6, 0, 0, 0 });
        // Через год и три с половиной месяца (XXX в этом тесте ломается таймзона, т.к. TSimpleTM не правильно делает Add(), поэтому сравниваем все время без таймзоны)
        m << TOneMock<TStringSlot>(R"({ "months": 3, "years": 1, "weeks": 2, "months_relative": true, "years_relative": true, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2018, 5, 19, 11, 25, 53 });

        // XXX не работает, надо отдавать ошибку на 32 число
        //m << TOneMock((R"({ "days": 32 })", cepoch, 1488273953);
        m.TestIt();
    }

    Y_UNIT_TEST(OneSpecificDayWithoutLookForwardJson) {
        // Sun Feb  5 11:25:53 EET 2017
        time_t e = 1486286753;
        TOneDayMocker<TJsonSlot> m("Europe/Kiev", false);

        // завтра
        m << TOneMock<TJsonSlot>(R"({ "days": 1, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 6, 11, 25, 53 });
        // послезавтра
        m << TOneMock<TJsonSlot>(R"({ "days": 2, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 7, 11, 25, 53 });
        // второго (имеется ввиду второго числа этого месяца этого года с обнуленным временем)  (2 feb 2017)
        m << TOneMock<TJsonSlot>(R"({ "days": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 2, 0, 0, 0 });
        // на следующей неделе (Sun Feb 12 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weeks": 1, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 12, 11, 25, 53 });
        // на этой неделе (это уже datetime_range, но мы сработаем текущей пользовательской датой)
        m << TOneMock<TJsonSlot>(R"({ "weeks": 0, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // 10 мая 2016 (не забываем обнулять время)
        m << TOneMock<TJsonSlot>(R"({ "years": 2016, "months": 5, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2016, 5, 10, 0, 0, 0 });
        // 10 мая 2016 в 12:30
        m << TOneMock<TJsonSlot>(R"({ "years": 2016, "months": 5, "days": 10, "hours": 12, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2016, 5, 10, 12, 30, 0 });
        // 20 марта 2016 в 19 часов
        m << TOneMock<TJsonSlot>(R"({ "years": 2016, "months": 3, "days": 20, "hours": 19 })", e, TDateTime::EDayPart::Evening, { 2016, 3, 20, 19, 0, 0 });
        // 10го числа следующего месяца (Tue May 10 12:25:53 MSK 2016)
        m << TOneMock<TJsonSlot>(R"({ "months": 1, "months_relative": true, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2017, 3, 10, 0, 0, 0 });
        // в 16:30 (Sun Feb  5 16:30:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "hours": 16, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 16, 30, 0 });
        // через два часа (Sun Feb  5 13:00:00 2017)
        m << TOneMock<TJsonSlot>(R"({ "hours": 2, "hours_relative": true })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 13, 25, 53 });
        // в понедельник (Mon Jan  30 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 1 })", e, TDateTime::EDayPart::WholeDay, { 2017, 1, 30, 11, 25, 53 });
        // во вторник (Tue Jan  31 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 1, 31, 11, 25, 53 });
        // в среду (Wed Feb  1 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 3 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 1, 11, 25, 53 });
        // в четверг (Thu Feb  2 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 4 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 2, 11, 25, 53 });
        // в пятницу (Fri Feb 3 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 5 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 3, 11, 25, 53 });
        // в субботу (Sat Feb 4 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 6 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 4, 11, 25, 53 });
        // в субботу в полночь (Sat Feb  04 00:00:00 MSK 2017) FIXME actually it is incorrect, must be Feb 05 00:00:00
        m << TOneMock<TJsonSlot>(R"({ "weekday": 6, "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 4, 0, 0, 0 });
        // в воскресенье; и т.к. сегодня воскресенье - возвращаем сегодня! (Sun Feb  5 12:25:53 MSK 2017)
        m << TOneMock<TJsonSlot>(R"({ "weekday": 7 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // Вчера в пятнадцать минут первого ночи
        m << TOneMock<TJsonSlot>(R"({ "days": -1, "days_relative": true, "hours": 0, "minutes": 15 })", e, TDateTime::EDayPart::Night, { 2017, 2, 4, 0, 15, 0 });
        // через 3 часа, 9 минут, 15 секунд
        m << TOneMock<TJsonSlot>(R"({ "time_relative": true, "hours": 3, "minutes": 9, "seconds": 15 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 14, 35, 8 });
        // в полночь (НЕ прибавляем один день, т.к. не lookforawd)
        m << TOneMock<TJsonSlot>(R"({ "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 5, 0, 0, 0 });
        // Через год и три с половиной месяца (XXX в этом тесте ломается таймзона, т.к. TSimpleTM не правильно делает Add(), поэтому сравниваем все время без таймзоны)
        m << TOneMock<TJsonSlot>(R"({ "months": 3, "years": 1, "weeks": 2, "months_relative": true, "years_relative": true, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2018, 5, 19, 11, 25, 53 });
        // Два года три месяца назад
        m << TOneMock<TJsonSlot>(R"({ "months": -3, "years": -2, "months_relative": true, "years_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2014, 11, 5, 11, 25, 53 });

        m.TestIt();
    }

    Y_UNIT_TEST(OneSpecificDayWithoutLookForwardString) {
        // Sun Feb  5 11:25:53 EET 2017
        time_t e = 1486286753;
        TOneDayMocker<TStringSlot> m("Europe/Kiev", false);

        // завтра
        m << TOneMock<TStringSlot>(R"({ "days": 1, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 6, 11, 25, 53 });
        // послезавтра
        m << TOneMock<TStringSlot>(R"({ "days": 2, "days_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 7, 11, 25, 53 });
        // второго (имеется ввиду второго числа этого месяца этого года с обнуленным временем)  (2 feb 2017)
        m << TOneMock<TStringSlot>(R"({ "days": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 2, 0, 0, 0 });
        // на следующей неделе (Sun Feb 12 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weeks": 1, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 12, 11, 25, 53 });
        // на этой неделе (это уже datetime_range, но мы сработаем текущей пользовательской датой)
        m << TOneMock<TStringSlot>(R"({ "weeks": 0, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // 10 мая 2016 (не забываем обнулять время)
        m << TOneMock<TStringSlot>(R"({ "years": 2016, "months": 5, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2016, 5, 10, 0, 0, 0 });
        // 10 мая 2016 в 12:30
        m << TOneMock<TStringSlot>(R"({ "years": 2016, "months": 5, "days": 10, "hours": 12, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2016, 5, 10, 12, 30, 0 });
        // 20 марта 2016 в 19 часов
        m << TOneMock<TStringSlot>(R"({ "years": 2016, "months": 3, "days": 20, "hours": 19 })", e, TDateTime::EDayPart::Evening, { 2016, 3, 20, 19, 0, 0 });
        // 10го числа следующего месяца (Tue May 10 12:25:53 MSK 2016)
        m << TOneMock<TStringSlot>(R"({ "months": 1, "months_relative": true, "days": 10 })", e, TDateTime::EDayPart::WholeDay, { 2017, 3, 10, 0, 0, 0 });
        // в 16:30 (Sun Feb  5 16:30:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "hours": 16, "minutes": 30 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 16, 30, 0 });
        // через два часа (Sun Feb  5 13:00:00 2017)
        m << TOneMock<TStringSlot>(R"({ "hours": 2, "hours_relative": true })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 13, 25, 53 });
        // в понедельник (Mon Jan  30 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 1 })", e, TDateTime::EDayPart::WholeDay, { 2017, 1, 30, 11, 25, 53 });
        // во вторник (Tue Jan  31 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 2 })", e, TDateTime::EDayPart::WholeDay, { 2017, 1, 31, 11, 25, 53 });
        // в среду (Wed Feb  1 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 3 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 1, 11, 25, 53 });
        // в четверг (Thu Feb  2 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 4 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 2, 11, 25, 53 });
        // в пятницу (Fri Feb 3 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 5 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 3, 11, 25, 53 });
        // в субботу (Sat Feb 4 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 6 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 4, 11, 25, 53 });
        // в субботу в полночь (Sat Feb  04 00:00:00 MSK 2017) FIXME actually it is incorrect, must be Feb 05 00:00:00
        m << TOneMock<TStringSlot>(R"({ "weekday": 6, "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 4, 0, 0, 0 });
        // в воскресенье; и т.к. сегодня воскресенье - возвращаем сегодня! (Sun Feb  5 12:25:53 MSK 2017)
        m << TOneMock<TStringSlot>(R"({ "weekday": 7 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 5, 11, 25, 53 });
        // Вчера в пятнадцать минут первого ночи
        m << TOneMock<TStringSlot>(R"({ "days": -1, "days_relative": true, "hours": 0, "minutes": 15 })", e, TDateTime::EDayPart::Night, { 2017, 2, 4, 0, 15, 0 });
        // через 3 часа, 9 минут, 15 секунд
        m << TOneMock<TStringSlot>(R"({ "time_relative": true, "hours": 3, "minutes": 9, "seconds": 15 })", e, TDateTime::EDayPart::Day, { 2017, 2, 5, 14, 35, 8 });
        // в полночь (НЕ прибавляем один день, т.к. не lookforawd)
        m << TOneMock<TStringSlot>(R"({ "hours": 0, "minutes": 0 })", e, TDateTime::EDayPart::Night, { 2017, 2, 5, 0, 0, 0 });
        // Через год и три с половиной месяца (XXX в этом тесте ломается таймзона, т.к. TSimpleTM не правильно делает Add(), поэтому сравниваем все время без таймзоны)
        m << TOneMock<TStringSlot>(R"({ "months": 3, "years": 1, "weeks": 2, "months_relative": true, "years_relative": true, "weeks_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2018, 5, 19, 11, 25, 53 });
        // Два года три месяца назад
        m << TOneMock<TStringSlot>(R"({ "months": -3, "years": -2, "months_relative": true, "years_relative": true })", e, TDateTime::EDayPart::WholeDay, { 2014, 11, 5, 11, 25, 53 });

        m.TestIt();
    }

    Y_UNIT_TEST(ShrinkRestDateTimeJson) {
        TOneDayMocker<TJsonSlot> m("Etc/GMT", false);

        // Sun Feb  5 09:25:53 GMT 2017
        time_t e = 1486286753;
        m << TOneMock<TJsonSlot>(R"({ "years": 2018 })", e, TDateTime::EDayPart::WholeDay, { 2018, 1, 1, 0, 0, 0 });
        m << TOneMock<TJsonSlot>(R"({ "years": 2018, "months": 5 })", e, TDateTime::EDayPart::WholeDay, { 2018, 5, 1, 0, 0, 0 });
        m << TOneMock<TJsonSlot>(R"({ "months": 5 })", e, TDateTime::EDayPart::WholeDay, { 2017, 5, 1, 0, 0, 0 });
        m << TOneMock<TJsonSlot>(R"({ "days": 25 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 25, 0, 0, 0 });
        m.TestIt();
    }

    Y_UNIT_TEST(ShrinkRestDateTimeString) {
        TOneDayMocker<TStringSlot> m("Etc/GMT", false);

        // Sun Feb  5 09:25:53 GMT 2017
        time_t e = 1486286753;
        m << TOneMock<TStringSlot>(R"({ "years": 2018 })", e, TDateTime::EDayPart::WholeDay, { 2018, 1, 1, 0, 0, 0 });
        m << TOneMock<TStringSlot>(R"({ "years": 2018, "months": 5 })", e, TDateTime::EDayPart::WholeDay, { 2018, 5, 1, 0, 0, 0 });
        m << TOneMock<TStringSlot>(R"({ "months": 5 })", e, TDateTime::EDayPart::WholeDay, { 2017, 5, 1, 0, 0, 0 });
        m << TOneMock<TStringSlot>(R"({ "days": 25 })", e, TDateTime::EDayPart::WholeDay, { 2017, 2, 25, 0, 0, 0 });
        m.TestIt();
    }

    Y_UNIT_TEST(RangeFromToJson) {
        TJsonSlot slot("when", "string");
        slot.Type = "datetime_range";
        slot.Value = NSc::TValue::FromJson(R"({ start: { "days": 0, "days_relative": true }, end: { "days": 2, "days_relative": true } })");

        std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
            { 10 }
        );
        UNIT_ASSERT(dtr);
        UNIT_ASSERT(!dtr->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 2);
        size_t days = 0;
        NDatetime::TSimpleTM curTime = NDatetime::ToCivilTime(TInstant::Seconds(Epochs()[0].first), Tz());
        for (const TDateTime& dt : *dtr) {
            ++days;
            UNIT_ASSERT_VALUES_EQUAL(dt.DayPart(), TDateTime::EDayPart::WholeDay);
            UNIT_ASSERT_VALUES_EQUAL(dt.SplitTime().ToString("%F"), curTime.ToString("%F"));
            curTime.Add(NDatetime::TSimpleTM::EField::F_DAY, 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(days, 2);
    }

    Y_UNIT_TEST(RangeFromToString) {
        TStringSlot slot("when", "string");
        slot.Type = "datetime_range";
        slot.Value = R"({ start: { "days": 0, "days_relative": true }, end: { "days": 2, "days_relative": true } })";

        std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)),
            { 10 }
        );
        UNIT_ASSERT(dtr);
        UNIT_ASSERT(!dtr->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 2);
        size_t days = 0;
        NDatetime::TSimpleTM curTime = NDatetime::ToCivilTime(TInstant::Seconds(Epochs()[0].first), Tz());
        for (const TDateTime& dt : *dtr) {
            ++days;
            UNIT_ASSERT_VALUES_EQUAL(dt.DayPart(), TDateTime::EDayPart::WholeDay);
            UNIT_ASSERT_VALUES_EQUAL(dt.SplitTime().ToString("%F"), curTime.ToString("%F"));
            curTime.Add(NDatetime::TSimpleTM::EField::F_DAY, 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(days, 2);
    }

    Y_UNIT_TEST(RangeDifferentTypeJson) {
        TJsonSlot slot("when", "datetime_range");
        // (сейчас воскр) погода с завтра по субботу
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "days": 1, "days_relative": true }, "end": { "weekday": 6 } })");

        std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)), // Sun Feb  5 12:25:53 MSK 2017
            { 10, true }
        );
        UNIT_ASSERT(dtr);
        UNIT_ASSERT(!dtr->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 6);
        size_t days = 0;
        NDatetime::TSimpleTM curTime = NDatetime::ToCivilTime(TInstant::Seconds(Epochs()[0].first), Tz());
        curTime.Add(NDatetime::TSimpleTM::EField::F_DAY, 1);
        for (const TDateTime& dt : *dtr) {
            ++days;
            UNIT_ASSERT_VALUES_EQUAL(dt.DayPart(), TDateTime::EDayPart::WholeDay);
            UNIT_ASSERT_VALUES_EQUAL(dt.SplitTime().ToString("%F"), curTime.ToString("%F"));
            curTime.Add(NDatetime::TSimpleTM::EField::F_DAY, 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(days, 6);
    }

    Y_UNIT_TEST(RangeDifferentTypeString) {
        TStringSlot slot("when", "datetime_range");
        // (сейчас воскр) погода с завтра по субботу
        slot.Value = R"({ "start": { "days": 1, "days_relative": true }, "end": { "weekday": 6 } })";

        std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)), // Sun Feb  5 12:25:53 MSK 2017
            { 10, true }
        );
        UNIT_ASSERT(dtr);
        UNIT_ASSERT(!dtr->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), 6);
        size_t days = 0;
        NDatetime::TSimpleTM curTime = NDatetime::ToCivilTime(TInstant::Seconds(Epochs()[0].first), Tz());
        curTime.Add(NDatetime::TSimpleTM::EField::F_DAY, 1);
        for (const TDateTime& dt : *dtr) {
            ++days;
            UNIT_ASSERT_VALUES_EQUAL(dt.DayPart(), TDateTime::EDayPart::WholeDay);
            UNIT_ASSERT_VALUES_EQUAL(dt.SplitTime().ToString("%F"), curTime.ToString("%F"));
            curTime.Add(NDatetime::TSimpleTM::EField::F_DAY, 1);
        }
        UNIT_ASSERT_VALUES_EQUAL(days, 6);
    }

    Y_UNIT_TEST(RangeWeekendsJson) {
        for (const auto& epoch : Epochs()) {
            TJsonSlot slot("when", "datetime_range");
            slot.Value = NSc::TValue::FromJson(R"({ "start": { "weekend": true} })");
            std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TJsonSlot>(
                &slot, nullptr,
                TDateTime(TDateTime::TSplitTime(Tz(), epoch.first)),
                { 10 }
            );

            UNIT_ASSERT(dtr);
            UNIT_ASSERT(!dtr->IsNow());

            size_t numDays = epoch.second.WDay == 0 ? 1 : 2;
            UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), numDays);
            for (const auto& day : *dtr) {
                UNIT_ASSERT_VALUES_EQUAL(day.DayPart(), TDateTime::EDayPart::WholeDay);
            }
            // TODO add more checks for days
        }
    }

    Y_UNIT_TEST(RangeWeekendsString) {
        for (const auto& epoch : Epochs()) {
            TStringSlot slot("when", "datetime_range");
            slot.Value = R"({ "start": { "weekend": true} })";
            std::unique_ptr<TDateTimeList> dtr = TDateTimeList::CreateFromSlot<TStringSlot>(
                &slot, nullptr,
                TDateTime(TDateTime::TSplitTime(Tz(), epoch.first)),
                { 10 }
            );

            UNIT_ASSERT(dtr);
            UNIT_ASSERT(!dtr->IsNow());

            size_t numDays = epoch.second.WDay == 0 ? 1 : 2;
            UNIT_ASSERT_VALUES_EQUAL(dtr->TotalDays(), numDays);
            for (const auto& day : *dtr) {
                UNIT_ASSERT_VALUES_EQUAL(day.DayPart(), TDateTime::EDayPart::WholeDay);
            }
            // TODO add more checks for days
        }
    }

    Y_UNIT_TEST(RangeWeekdaysPastJson) {
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weekday": 4 }, "end": { "weekday": 6 } })");
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)), // Sun Feb  5 12:25:53 MSK 2017
            { 10 }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 3);
    }

    Y_UNIT_TEST(RangeWeekdaysPastString) {
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weekday": 4 }, "end": { "weekday": 6 } })";
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(TDateTime::TSplitTime(Tz(), Epochs()[0].first)), // Sun Feb  5 12:25:53 MSK 2017
            { 10 }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 3);
    }

    Y_UNIT_TEST(RangeWeekdaysCurrentToSpecifiedJson) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weekday": 5 }, "end": { "weekday": 7 } })");
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 3);
        // since 5 weekday is the next day from cur, so ajust it!
        cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        for (const auto& d : *dtl) {
            TStringBuilder w;
            w << "test: " << d.SplitTime().ToString("%F") << ", etalon: " << cur.ToString("%F");
            UNIT_ASSERT_EQUAL_C(d.SplitTime().ToString("%F"), cur.ToString("%F"), w.data());
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeWeekdaysCurrentToSpecifiedString) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weekday": 5 }, "end": { "weekday": 7 } })";
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 3);
        // since 5 weekday is the next day from cur, so ajust it!
        cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        for (const auto& d : *dtl) {
            TStringBuilder w;
            w << "test: " << d.SplitTime().ToString("%F") << ", etalon: " << cur.ToString("%F");
            UNIT_ASSERT_EQUAL_C(d.SplitTime().ToString("%F"), cur.ToString("%F"), w.data());
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeWeekdaysNextToSpecifiedJson) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weekday": 5 }, "end": { "weekday": 6 } })");
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 2);

        cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        for (const auto& d : *dtl) {
            UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), cur.ToString("%F"));
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeWeekdaysNextToSpecifiedString) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weekday": 5 }, "end": { "weekday": 6 } })";
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 2);

        cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        for (const auto& d : *dtl) {
            UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), cur.ToString("%F"));
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    /* not implemented yet TODO
    Y_UNIT_TEST(RangeWeeksAbsolute) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "years": 2016, "weeks": 0 }, "end": { "years": 2016, "weeks": 2 } })");
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }, nullptr
        );
    }
    */


    Y_UNIT_TEST(RangeWeekdaysNextToNextWeekJson) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weekday": 5 }, "end": { "weekday": 4 } })");
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);

        cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        for (const auto& d : *dtl) {
            UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), cur.ToString("%F"));
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeWeekdaysNextToNextWeekString) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weekday": 5 }, "end": { "weekday": 4 } })";
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );
        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);

        cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        for (const auto& d : *dtl) {
            UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), cur.ToString("%F"));
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeFromNowForWeekJson) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weeks": 0, "weeks_relative": true }, "end": { "weeks": 1, "weeks_relative": true } })");
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );

        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);
        for (const auto& d : *dtl) {
            UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), cur.ToString("%F"));
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeFromNowForWeekString) {
        // Thu Mar  2 15:28:14 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488457694);
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weeks": 0, "weeks_relative": true }, "end": { "weeks": 1, "weeks_relative": true } })";
        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );

        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);
        for (const auto& d : *dtl) {
            UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), cur.ToString("%F"));
            cur.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
        }
    }

    Y_UNIT_TEST(RangeNextWeekJson) {
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weeks": 1, "weeks_relative": true }, "end": { "weeks": 2, "weeks_relative": true } })");
        for (size_t day = 0; day < 7; ++day) {
            TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488198494 + 86400 * day);

            std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
                &slot, nullptr,
                TDateTime(cur),
                { 10, true }
            );

            TDateTime::TSplitTime canon(NDatetime::GetTimeZone("Europe/Moscow"), 1488803294);

            UNIT_ASSERT(dtl);
            UNIT_ASSERT(!dtl->IsNow());
            UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);
            for (const auto& d : *dtl) {
                UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), canon.ToString("%F"));
                canon.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
            }
        }
    }

    Y_UNIT_TEST(RangeNextWeekString) {
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weeks": 1, "weeks_relative": true }, "end": { "weeks": 2, "weeks_relative": true } })";
        for (size_t day = 0; day < 7; ++day) {
            TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1488198494 + 86400 * day);

            std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
                &slot, nullptr,
                TDateTime(cur),
                { 10, true }
            );

            TDateTime::TSplitTime canon(NDatetime::GetTimeZone("Europe/Moscow"), 1488803294);

            UNIT_ASSERT(dtl);
            UNIT_ASSERT(!dtl->IsNow());
            UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);
            for (const auto& d : *dtl) {
                UNIT_ASSERT_VALUES_EQUAL(d.SplitTime().ToString("%F"), canon.ToString("%F"));
                canon.Add(TDateTime::TSplitTime::EField::F_DAY, 1);
            }
        }
    }

    Y_UNIT_TEST(FromTodayWeekDayToNextWeekPrevJson) {
        // Fri Mar 10 13:49:56 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1489142996);
        TJsonSlot slot("when", "datetime_range");
        slot.Value = NSc::TValue::FromJson(R"({ "start": { "weekday": 5 }, "end": { "weekday": 4 } })");

        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );

        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);
        UNIT_ASSERT_VALUES_EQUAL(dtl->cbegin()->SplitTime().ToString("%F"), "2017-03-10");
        UNIT_ASSERT_VALUES_EQUAL(dtl->rbegin()->SplitTime().ToString("%F"), "2017-03-16");
    }

    Y_UNIT_TEST(FromTodayWeekDayToNextWeekPrevString) {
        // Fri Mar 10 13:49:56 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1489142996);
        TStringSlot slot("when", "datetime_range");
        slot.Value = R"({ "start": { "weekday": 5 }, "end": { "weekday": 4 } })";

        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );

        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 7);
        UNIT_ASSERT_VALUES_EQUAL(dtl->cbegin()->SplitTime().ToString("%F"), "2017-03-10");
        UNIT_ASSERT_VALUES_EQUAL(dtl->rbegin()->SplitTime().ToString("%F"), "2017-03-16");
    }

    Y_UNIT_TEST(OldDatesToTimeTJson) {
        // Fri Mar 10 13:49:56 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1489142996);
        TJsonSlot slot("when", "datetime");
        slot.Value = NSc::TValue::FromJson(R"({ "years": -55, "years_relative": true })");

        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TJsonSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );

        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 1);
        UNIT_ASSERT_VALUES_EQUAL(dtl->cbegin()->SplitTime().ToString("%F"), "1962-03-10");
        UNIT_ASSERT_VALUES_EQUAL(dtl->cbegin()->SplitTime().AsTimeT(), 0);
    }

    Y_UNIT_TEST(OldDatesToTimeTString) {
        // Fri Mar 10 13:49:56 MSK 2017
        TDateTime::TSplitTime cur(NDatetime::GetTimeZone("Europe/Moscow"), 1489142996);
        TStringSlot slot("when", "datetime");
        slot.Value = R"({ "years": -55, "years_relative": true })";

        std::unique_ptr<TDateTimeList> dtl = TDateTimeList::CreateFromSlot<TStringSlot>(
            &slot, nullptr,
            TDateTime(cur),
            { 10, true }
        );

        UNIT_ASSERT(dtl);
        UNIT_ASSERT(!dtl->IsNow());
        UNIT_ASSERT_VALUES_EQUAL(dtl->TotalDays(), 1);
        UNIT_ASSERT_VALUES_EQUAL(dtl->cbegin()->SplitTime().ToString("%F"), "1962-03-10");
        UNIT_ASSERT_VALUES_EQUAL(dtl->cbegin()->SplitTime().AsTimeT(), 0);
    }

    // TODO
    // добавить тест с пустым datetime_range
    // с пустым слотом datetime и установленным daypart = 'night'
}
