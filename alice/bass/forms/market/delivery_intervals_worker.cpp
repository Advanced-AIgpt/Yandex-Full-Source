#include "delivery_intervals_worker.h"
#include "forms.h"
#include "settings.h"

#include <alice/bass/forms/market.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/datetime/datetime.h>
#include <alice/library/scenarios/alarm/date_time.h>
#include <util/draft/date.h>
#include <util/generic/algorithm.h>
#include <regex>

using NAlice::TDateTime;

namespace NBASS {

namespace NMarket {

namespace {

const std::regex DATE_REGEX(R"(\d{2}-\d{2}-\d{4})");
const std::regex TIME_REGEX(R"(\d{2}:\d{2})");

}

TDeliveryIntervalsWorker::TDeliveryIntervalsWorker(TMarketContext& ctx)
    : Ctx(ctx)
{
}

TMaybe<i64> TDeliveryIntervalsWorker::Handle(TState& state)
{
    if (IsSlotEmpty(Ctx.Ctx().GetSlot("date"))) {
        LOG(DEBUG) << "Slot date is empty" << Endl;
        return Nothing();
    }
    if (state.DeliveryOptions().Empty()) {
        LOG(DEBUG) << "Delivery options provided by checkouter are empty" << Endl;
        return Nothing();
    }
    const TDateTime userTime(
        TDateTime::TSplitTime(
            NDatetime::GetTimeZone(Ctx.Ctx().UserTimeZone()),
            Ctx.Meta().Epoch()
        )
    );

    const auto& leftBound = GetDeliveryLeftBound(state);

    TDeliveryIntervalsContext context(TVector<bool>(state.DeliveryOptions().Size(), true), leftBound, userTime);

    FilterOptionsByDay(state, context);
    FilterOptionsByDateTime(state, Ctx.Ctx().GetSlot(TStringBuf("time_from")), context, true /* compareFrom */);
    FilterOptionsByDateTime(state, Ctx.Ctx().GetSlot(TStringBuf("time_to")), context, false /* compareFrom */);

    const auto& suitableOptionsCount = Count(
        context.FilteredDeliveryOptions.begin(),
        context.FilteredDeliveryOptions.end(),
        true
    );

    if (suitableOptionsCount != 1) {
        LOG(DEBUG) << suitableOptionsCount << " options found for request, it should be exactly 1" << Endl;
        return Nothing();
    }
    return FindIndex(context.FilteredDeliveryOptions, true) + 1;
}

TDateTime TDeliveryIntervalsWorker::GetDate(
    const NSc::TValue& date,
    const TDateTime& userDateTime,
    const TDateTime::TSplitTime& leftBound)
{
    TDateTime::EDayPart dayPart = TDateTime::EDayPart::WholeDay;
    const NAlice::TDateTimeListCreationParams& params = {20 /* maxDays */, false /* lookForward */};
    NAlice::TDaysParser daysParser(&params, false /* isRange */);
    return daysParser.ParseOneDay(date, userDateTime, &dayPart, &leftBound);
}

void TDeliveryIntervalsWorker::FilterOptionsByDay(const TState& state, TDeliveryIntervalsContext& context)
{
    const NSc::TValue& dateSlotValue = Ctx.Ctx().GetSlot(TStringBuf("date"))->Value;
    const auto& userDate = GetDate(dateSlotValue, context.UserTime, context.LeftBound);

    for (size_t i = 0; i < state.DeliveryOptions().Size(); ++i) {
        const auto& deliveryOption = state.DeliveryOptions(i).Dates();
        const auto& checkoutDateTime = ParseSplitTime(deliveryOption.FromDate(), deliveryOption.FromTime());

        if (!CompareDeliveryDates(checkoutDateTime, userDate, dateSlotValue)) {
            context.FilteredDeliveryOptions[i] = false;
        }
    }
}

void TDeliveryIntervalsWorker::FilterOptionsByDateTime(
    const TState& state,
    TContext::TSlot* dateTimeSlot,
    TDeliveryIntervalsContext& context,
    bool compareFrom)
{
    if (IsSlotEmpty(dateTimeSlot)) {
        return;
    }
    dateTimeSlot->Value.MergeUpdate(Ctx.Ctx().GetSlot(TStringBuf("date"))->Value);
    const auto& userDateTime = GetDate(dateTimeSlot->Value, context.UserTime, context.LeftBound);
    for (size_t i = 0; i < state.DeliveryOptions().Size(); ++i) {
        const auto& deliveryOption = state.DeliveryOptions(i).Dates();

        const auto& checkoutDateTime = compareFrom
            ? ParseSplitTime(deliveryOption.FromDate(), deliveryOption.FromTime())
            : ParseSplitTime(deliveryOption.ToDate(), deliveryOption.ToTime());

        if (!CompareDeliveryDateTimes(checkoutDateTime, userDateTime, dateTimeSlot->Value)) {
            context.FilteredDeliveryOptions[i] = false;
        }
    }
}

/**
 * @param date - date formatted "dd-mm-yyyy"
 * @param time - time formatted "HH:MM"
 */
TDateTime::TSplitTime TDeliveryIntervalsWorker::ParseSplitTime(TStringBuf date, TStringBuf time)
{
    Y_ASSERT(std::regex_match(date.begin(), date.end(), DATE_REGEX));
    Y_ASSERT(std::regex_match(time.begin(), time.end(), TIME_REGEX));
    const auto& year = FromString<ui32>(date.SubString(6, 4));
    const auto& month = FromString<ui32>(date.SubString(3, 2));
    const auto& day = FromString<ui32>(date.SubString(0, 2));
    const auto& hour = FromString<ui32>(time.SubString(0, 2));
    const auto& minutes = FromString<ui32>(time.SubString(3, 2));

    return TDateTime::TSplitTime(
        NDatetime::GetTimeZone(Ctx.Ctx().UserTimeZone()),
        year, month, day, hour, minutes
    );
}

TDateTime::TSplitTime TDeliveryIntervalsWorker::GetDeliveryLeftBound(const TState& state)
{
    TDateTime::TSplitTime result = ParseSplitTime(state.DeliveryOptions()[0].Dates().FromDate(),
        state.DeliveryOptions()[0].Dates().FromTime());
    for (const auto& currDeliveryTime : state.DeliveryOptions()) {
        const auto& currDateTime = ParseSplitTime(currDeliveryTime.Dates().FromDate(), currDeliveryTime.Dates().FromTime());
        if (currDateTime.AsTimeT() < result.AsTimeT()) {
            result = currDateTime;
        }
    }
    return result;
}

bool TDeliveryIntervalsWorker::CompareDeliveryDateTimes(
    const TDateTime::TSplitTime& checkouterDateTime,
    const TDateTime& userDateTime,
    const NSc::TValue& dateTimeSlot)
{
    const auto& splitTime = userDateTime.SplitTime();
    return CompareDeliveryDates(checkouterDateTime, userDateTime, dateTimeSlot)
           && splitTime.Hour() % 12 == checkouterDateTime.Hour() % 12
           && splitTime.Min() == checkouterDateTime.Min();
}

bool TDeliveryIntervalsWorker::CompareDeliveryDates(
    const TDateTime::TSplitTime& checkouterDateTime,
    const TDateTime& userDate,
    const NSc::TValue& dateTimeSlot)
{
    const auto& splitTime = userDate.SplitTime();
    bool dayEquals = splitTime.MDay() == checkouterDateTime.MDay();
    if (dateTimeSlot.Has(TStringBuf("weekday")) && !dateTimeSlot.Has(TStringBuf("days"))) {
        dayEquals = splitTime.WDay() == checkouterDateTime.WDay();
    }

    return splitTime.RealYear() == checkouterDateTime.RealYear()
           && splitTime.RealMonth() == checkouterDateTime.RealMonth()
           && dayEquals;
}

}

}
