#include "calendar_utils.h"

namespace NAlice::NHollywoodFw::NGetDate {

namespace {

struct TWinnerHelper {
    TCalendarDatesArray::EDateType DateType;
    int OccurenceCount = 0;
    size_t FirstItemInArray = 0;
};

} // Anonimous namespace

/*
    TCalendarDatesArray ctor
    Fill all dates information using current frame state
*/
TCalendarDatesArray::TCalendarDatesArray(const TGetDateSceneArgs& sceneArgs) {
    for (const auto& it : sceneArgs.GetDate()) {
        TMaybe<TSysDatetimeParser> calendarDate = TSysDatetimeParser::Parse(it);
        if (calendarDate.Defined()) {
            CalendarDates_.push_back({*calendarDate, calendarDate->GetParseInfo(), NDatetime::TCivilSecond{0}});
        }
    }
}

void TCalendarDatesArray::AddToday() {
    const TSysDatetimeParser today = *TSysDatetimeParser::Today();
    for (const auto& it: CalendarDates_) {
        if (it.DateSlot.GetRawDatetime() == today.GetRawDatetime()) {
            // Already has today
            return;
        }
    }
    CalendarDates_.push_back({today, today.GetParseInfo(), NDatetime::TCivilSecond{0}});
}

/*
    Find a winner in slots
    @params hypothesis - array of possible slot types in TCalendarDatesArray
    conditions:
        - TCalendarDatesArray MUST NOT contain any datetypes not exists in hypothesis
        - all hypothesis MUST EXIST in TCalendarDatesArray
        - hypothesis[0] must occur 1 time only
    @return:
        - nullptr if condition failed
        - pointer to the first date slot for the zero item in hypothesis
*/
TCalendarDatesArray::TDateInformation* TCalendarDatesArray::FindWinner(const TVector<EDateType>& hypothesis) {
    if (hypothesis.size() > CalendarDates_.size()) {
        return nullptr;
    }

    // Create TWinnerHelper array
    TVector<TWinnerHelper> hInfo;
    hInfo.resize(hypothesis.size());
    for (size_t i = 0; i < hypothesis.size(); i++) {
        hInfo[i].DateType = hypothesis[i];
    }

    // Handle all dates in source array
    for (size_t i = 0; i < CalendarDates_.size(); i++) {
        const auto& d = CalendarDates_[i];
        auto* currentHyp = FindIfPtr(hInfo, [d](const auto& it) {
            return d.IsTypeOf(it.DateType);
        });
        if (currentHyp == nullptr) {
            // TCalendarDatesArray contains date type incompatible with hypothesis
            return nullptr;
        }
        currentHyp->OccurenceCount++;
        if (currentHyp->FirstItemInArray == 0) {
            currentHyp->FirstItemInArray = i;
        }
    }

    // Check results
    for (const auto& it : hInfo) {
        if (it.OccurenceCount == 0) {
            return nullptr;
        }
    }
    return hInfo[0].OccurenceCount == 1 ? &At(hInfo[0].FirstItemInArray) : nullptr;
}

} // namespace NAlice::NHollywood::NGetDate
