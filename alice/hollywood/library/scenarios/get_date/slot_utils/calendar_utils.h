#pragma once

#include "slot_utils.h"

#include <alice/hollywood/library/scenarios/get_date/proto/get_date.pb.h>

#include <alice/library/sys_datetime/sys_datetime.h>

namespace NAlice::NHollywoodFw::NGetDate {

class TCalendarDatesArray {
public:
    enum class EDateType {
        Fixed,
        LargeRelative,
        Relative,
        Weekday,
        Unknown
    };
    /*
        Local structure to keep all dates information come from semantic frame or ScenarioState
    */
    struct TDateInformation {
        TSysDatetimeParser DateSlot;                    // Source date/time information
        TSysDatetimeParser::EDatetimeParser DateType;   // Cached type of DateSlot
        NDatetime::TCivilSecond DateResult;             // Result date calculated from DateSlot and Now()

        bool IsTypeOf(EDateType type) const {
            switch (type) {
                case EDateType::Fixed:
                    return DateType == TSysDatetimeParser::EDatetimeParser::Fixed;
                case EDateType::LargeRelative: {
                    if (DateType != TSysDatetimeParser::EDatetimeParser::RelativeFuture &&
                        DateType != TSysDatetimeParser::EDatetimeParser::RelativePast &&
                        DateType != TSysDatetimeParser::EDatetimeParser::RelativeMix) {
                        return false;
                    }
                    const auto& sourceDate = DateSlot.GetRawDatetime();
                    return (sourceDate.Year.IsFilled() && sourceDate.Year.Get() != 0) ||
                        (sourceDate.Month.IsFilled() && sourceDate.Month.Get() != 0) ||
                        (sourceDate.Day.IsFilled() && abs(sourceDate.Day.Get()) > 2);
                }
                case EDateType::Relative:
                    return DateType == TSysDatetimeParser::EDatetimeParser::RelativeFuture ||
                        DateType == TSysDatetimeParser::EDatetimeParser::RelativePast ||
                        DateType == TSysDatetimeParser::EDatetimeParser::RelativeMix;
                case EDateType::Weekday:
                    return DateType == TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek;
                case EDateType::Unknown:
                    break;
            }
            return DateType == TSysDatetimeParser::EDatetimeParser::Unknown ||
                   DateType == TSysDatetimeParser::EDatetimeParser::Mix;
        }
    };

    // For testing purpose only
    TCalendarDatesArray() {}
    // Default ctor from scenario proto
    explicit TCalendarDatesArray(const TGetDateSceneArgs& sceneArgs);
    void AddToday();

    void CalculateDestinationDates(const NDatetime::TCivilSecond dateCurrent, const TSysDatetimeParser::ETense tense) {
        for (auto& it : CalendarDates_) {
            it.DateResult = it.DateSlot.GetTargetDateTime(dateCurrent, tense);
        }
    }

    /*
        Compare all dates in vector and remove dublicates using external function
        fn: return false to comtinue comparing
            return true to remove second compared item (first item can be modified)
    */
    void CompactVector(bool (*fn) (TDateInformation& p1, const TDateInformation& p2)) {
        TVector<TDateInformation>::iterator it1;
        TVector<TDateInformation>::iterator it2;
        for (it1 = CalendarDates_.begin(); it1 != CalendarDates_.end(); ++it1) {
            for (it2 = it1 + 1; it2 != CalendarDates_.end(); ) {
                if (!fn(*it1, *it2)) {
                    ++it2;
                    continue;
                }
                it1->DateType = it1->DateSlot.GetParseInfo(); // Recalculate result
                it2 = CalendarDates_.erase(it2);
            }
        }
    }

    /*
        Amount of elements in TDateInformation array
    */
    size_t Size() const {
        return CalendarDates_.size();
    }

    // Direct access to TDateInformation array
    TDateInformation& At(size_t index) {
        return CalendarDates_[index];
    }

    // Get direct access to vector (for testing purpose only)
    TVector<TDateInformation>& Get() {
        return CalendarDates_;
    }

    // Merge data into one elements
    bool Merge(size_t index1, size_t index2) {
        if (At(index1).DateSlot.Merge(At(index2).DateSlot)) {
            At(index1).DateType = At(index1).DateSlot.GetParseInfo(); // Recalculate
            return true;
        }
        return false;
    }

    size_t FindByType(EDateType type) const {
        for (size_t i=1; i<CalendarDates_.size(); i++) {
            if (CalendarDates_[i].IsTypeOf(type)) {
                return i;
            }
        }
        return 0;
    }

    /*
        Проверка гипотез о наличии в массиве указанных элементов
        Массив hypothesis содержит образцы типов дат, которые проверяются в массиве CalendarDates_
        Условия выигрыша:
            * все варианты в массиве hypotesis встречаются как минимум 1 раз
            * нет вариантов дат, которые в массиве hypotesis не встречаются вообще
        Возвращается первый элемент типа hypotesis[0]
    */
    TDateInformation* FindWinner(const TVector<EDateType>& hypothesis);

    /*
        Check is all slots contains actual information
    */
    bool IsAllDatesMatched() const {
        for (size_t i=1; i<CalendarDates_.size(); i++) {
            if (CalendarDates_[i].DateResult != CalendarDates_[0].DateResult) {
                return false;
            }
        }
        return true;
    }

    /*
        Проверка гипотезы, что в каком то слоте TCalendarDatesArray дата совпадает с виннером
        @param dateResult указатель на слот выигрыша (FindWinner())

        Пример:     Сегодня пятница или суббота. Виннер: "Сегодня". Сравниваем "Сегодня" с пт и сб
                    Результат: ДА. Сегодня пятница (пт/сб нашлась в гипотезах пользователя)
                          или: НЕТ. Сегодня воскресенье (пт/сб отсутствует в гипотезах пользователя)
    */
    bool IsSomeDatesMatched(const TDateInformation* dateResult) const {
        for (const auto& it : CalendarDates_) {
            if (&it == dateResult) {
                // Skip winner slot
                continue;
            }
            switch (it.DateType) {
                case TSysDatetimeParser::EDatetimeParser::Unknown:
                case TSysDatetimeParser::EDatetimeParser::Mix:
                    // Can't check these slots
                    break;
                case TSysDatetimeParser::EDatetimeParser::Fixed:
                case TSysDatetimeParser::EDatetimeParser::RelativeFuture:
                case TSysDatetimeParser::EDatetimeParser::RelativePast:
                case TSysDatetimeParser::EDatetimeParser::RelativeMix:
                    if (it.DateResult == dateResult->DateResult) {
                        return true;
                    }
                    break;
                case TSysDatetimeParser::EDatetimeParser::FixedDayOfWeek: {
                    const NDatetime::TCivilDay day1 = NDatetime::TCivilDay{it.DateResult.year(), it.DateResult.month(), it.DateResult.day()};
                    const NDatetime::TCivilDay day2 = NDatetime::TCivilDay{dateResult->DateResult.year(), dateResult->DateResult.month(), dateResult->DateResult.day()};
                    if (NDatetime::GetWeekday(day1) == NDatetime::GetWeekday(day2)) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

private:
    TVector<TDateInformation> CalendarDates_;

};

}  // namespace NAlice::NHollywood::NGetDate
