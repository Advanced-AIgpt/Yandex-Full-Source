#include "getdate.h"

#include <alice/bass/libs/config/config.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/datetime/datetime.h>

#include <library/cpp/timezone_conversion/convert.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS {

namespace {

const size_t LAST_NON_LEAP_YEAR_DAY = 364;

class TDateRequestImpl {
public:
    TDateRequestImpl(TRequestHandler& r);
    TResultValue Do();

private:
    TContext& Ctx;
    TRequestedGeo Geo;
    NAlice::IRng& Rng;
    std::unique_ptr<TDateTime::TSplitTime> NowTimePtr;
    NSc::TValue AskedDate;
    size_t AskedYDay = 0;
    TString TimeZone;
    bool LookForward = true;
    bool HasRelative = false;
    bool WeekdayOnly = false;

    bool FindGeoAndTimeZone();
    bool CheckDateSlot(TContext::TSlot* slotDate) const;
    void InitAskedDate(NSc::TValue& value);
    bool AnalyzeRelative();
    void FixAskedDate();
    TResultValue CalculateOutputDate(NSc::TValue* res);
    void ConvertDateStructToOutputValue(const TDateTime::TSplitTime& date, NSc::TValue* res) const;
    void MakeSuggest();
};

TDateRequestImpl::TDateRequestImpl(TRequestHandler& r)
    : Ctx(r.Ctx())
    , Geo(Ctx.GlobalCtx())
    , Rng(Ctx.GetRng())
{
}

bool TDateRequestImpl::FindGeoAndTimeZone() {
    Geo = TRequestedGeo(Ctx, TStringBuf("where"));
    if (Geo.HasError()) {
        NSc::TValue er;
        if (Geo.GetError()->Type == TError::EType::NOUSERGEO) {
            er["code"] = TStringBuf("bad_user_geo");
        } else {
            er["code"] = TStringBuf("bad_geo");
            TContext::TSlot* slotWhere = Ctx.GetSlot("where");
            if (!IsSlotEmpty(slotWhere)) {
                er["where"] = slotWhere->Value.GetString();
                // reset slot (ASSISTANT-669)
                Ctx.CreateSlot("where", "string", true);
            }
        }
        Ctx.AddErrorBlock(*Geo.GetError(), std::move(er));
        return false;
    }

    TimeZone = Geo.GetTimeZone();
    if (TimeZone.empty()) {
        Ctx.AddErrorBlock(
            TError(
                TError::EType::NOTIMEZONE,
                TStringBuf("unknown_timezone")
            )
        );
        return false;
    }

    return true;
}

bool TDateRequestImpl::CheckDateSlot(TContext::TSlot* slotDate) const {
    if (IsSlotEmpty(slotDate)) {
        // Today as default
        NSc::TValue now;
        now["days"].SetNumber(0);
        now["days_relative"].SetBool(true);
        slotDate = Ctx.CreateSlot("calendar_date", "datetime", true, now);
        return true;
    }
    if (TStringBuf("datetime_raw") != slotDate->Type && TStringBuf("datetime") != slotDate->Type) {
        Ctx.AddErrorBlock(
            TError(
                TError::EType::INVALIDPARAM,
                TStringBuf("wrong_date_slot_type")
            )
        );
        return false;
    }
    return true;
}

bool TDateRequestImpl::AnalyzeRelative() {
    HasRelative = AskedDate.Get("date_relative").GetBool(false)
        || AskedDate.Get("days_relative").GetBool(false)
        || AskedDate.Get("months_relative").GetBool(false)
        || AskedDate.Get("years_relative").GetBool(false)
        || AskedDate.Get("hours_relative").GetBool(false);

    return HasRelative;
}

void TDateRequestImpl::InitAskedDate(NSc::TValue& value) {
    AskedDate = value;
    AnalyzeRelative();
}

void TDateRequestImpl::FixAskedDate() {
/*
    Available fields (withoth time and dateparts)
    days, days_relative
    weeks, weeks_relative
    months, months_relative
    years, years_relative
    weekday
*/
    // If query about day only,
    // check month (previous/current/next) using tense ('what day of week was 25').
    if (!HasRelative) {
        if (AskedDate["months"].IsNull() && AskedDate["years"].IsNull()) {
            if (!AskedDate["days"].IsNull()) {
                if (!LookForward && AskedDate["days"].GetIntNumber() > NowTimePtr->MDay()) {
                    AskedDate["months"].SetNumber(-1);
                    AskedDate["months_relative"].SetBool(true);
                } else if (LookForward && AskedDate["days"].GetIntNumber() < NowTimePtr->MDay()) {
                    AskedDate["months"].SetNumber(1);
                    AskedDate["months_relative"].SetBool(true);
                }
            } else if (!AskedDate["weekday"].IsNull()) {
                WeekdayOnly = true;
                if (!LookForward && AskedDate["weekday"].GetIntNumber() > NowTimePtr->WDay()) {
                    AskedDate["weeks"].SetNumber(-1);
                    AskedDate["weeks_relative"].SetBool(true);
                } else if (LookForward && AskedDate["weekday"].GetIntNumber() <= NowTimePtr->WDay()) {
                    AskedDate["weeks"].SetNumber(1);
                    AskedDate["weeks_relative"].SetBool(true);
                }
            }
        } else if (!AskedDate["months"].IsNull() && AskedDate["years"].IsNull()) {
            if (AskedDate["months_relative"].IsNull()) {
                size_t askedMonth = AskedDate["months"].GetIntNumber();
                size_t currentMonth = NowTimePtr->RealMonth();
                if (LookForward && askedMonth < currentMonth && (currentMonth - askedMonth > 6)) {
                    AskedDate["years"].SetNumber(1);
                    AskedDate["years_relative"].SetBool(true);
                } else if (!LookForward && askedMonth > currentMonth) {
                    AskedDate["years"].SetNumber(-1);
                    AskedDate["years_relative"].SetBool(true);
                }
            }
        }
    }
}

TResultValue TDateRequestImpl::CalculateOutputDate(NSc::TValue* res) {
    TResultValue dateError;
    std::unique_ptr<TDateTimeList> dtl;
    try {
        dtl = TDateTimeList::CreateDateTime<TContext::TSlot>(
            AskedDate,
            nullptr,
            TDateTime(*(NowTimePtr.get())),
            { 1, LookForward }
        );
    } catch (const yexception& e) {
        *dateError = TError(TError::EType::INVALIDPARAM, e.what());
        return dateError;
    }

    if (dtl) {
        if (dtl->TotalDays() == 1) {
            const TDateTime::TSplitTime& dst = dtl->cbegin()->SplitTime();
            AskedYDay = dst.YDay();
            ConvertDateStructToOutputValue(dst, res);
            return TResultValue();
        } else if (dtl->IsNow()) {
            AskedYDay = NowTimePtr->YDay();
            ConvertDateStructToOutputValue(*NowTimePtr, res);
            return TResultValue();
        }
    }

    // Error: wrong date format
    return TError(
        TError::EType::INVALIDPARAM,
        TStringBuf("invalid_date_format")
    );
}

void TDateRequestImpl::ConvertDateStructToOutputValue(const TDateTime::TSplitTime& date, NSc::TValue* res) const {
    res->SetDict();
    (*res)["year"] = date.RealYear();
    (*res)["month"] = date.RealMonth();
    (*res)["day"] = date.MDay();
    (*res)["wday"] = date.WDay() == 0 ? 7 : date.WDay();
    (*res)["yday"] = date.YDay();
    (*res)["timezone"] = date.TimeZone().name();
}

void TDateRequestImpl::MakeSuggest() {
    NSc::TValue suggest;
    ui32 todayYDay = NowTimePtr->YDay();

    // For absolute dates suggests relative dates
    if (!HasRelative && !WeekdayOnly) {
        // For queries about distant dates suggest tomorrow or day after tomorrow
        if ((AskedYDay > todayYDay && (AskedYDay - todayYDay > 2))
                || (AskedYDay < todayYDay && (LAST_NON_LEAP_YEAR_DAY - todayYDay + AskedYDay) > 2)) {
            // Second part of condition is needed for dismissing case when we ask about January, 1 and today is December, 31:
            // AskedYDay = 1, TodayYDay = 364(365)
            // In this case we must not offer "Tomorrow" for suggest as it is equal to user request.
            // It is ok for leap year
            ui32 day = 1 + Rng.RandomInteger(2); // tomorrow or day after tomorrow
            suggest["days"].SetNumber(day);
            suggest["days_relative"].SetBool(true);
        } else {
            // For nearby dates suggests next week
            ui32 wday = 3 + Rng.RandomInteger(5); // some weekday on the next week, wed(3) - sun(7)
            suggest["weekday"].SetNumber(wday);
            suggest["weeks"].SetNumber(1);
            suggest["weeks_relative"].SetBool(true);
        }
        Ctx.AddSuggest(TStringBuf("get_date__date_query"), std::move(suggest));
    } else {
        // For relative dates of queries about weekdays suggets absolute dates
        ui32 yday = todayYDay + 5 + Rng.RandomInteger(7); // some day in the future (5 - 11 days ahead)
        bool nextYear = false;
        if (yday > LAST_NON_LEAP_YEAR_DAY) { // next year
            yday = yday - LAST_NON_LEAP_YEAR_DAY - 1; // -1 for we can get 0 (January, 1)
            nextYear = true;
        }
        // If we randomly get date equal to user request, change it
        if (yday == AskedYDay) {
            // For beginning of year increase date, in other cases make it closer to now
            yday = yday < 5 ? yday + 2 : yday - 2;
        }
        ui32 suggestM = 0;
        ui32 suggestD = 0;

        //do not count leap years; we don't need to suggest February, 29
        NDatetime::YDayToMonthAndDay(yday, false, &suggestM, &suggestD);
        suggest["days"].SetNumber(suggestD);
        suggest["months"].SetNumber(++suggestM);
        if (nextYear) {
            suggest["years"].SetNumber(1);
            suggest["years_relative"].SetBool(true);
        }
        Ctx.AddSuggest(TStringBuf("get_date__day_query"), std::move(suggest));
    }

    Ctx.AddSearchSuggest();
    Ctx.AddOnboardingSuggest();
}

TResultValue TDateRequestImpl::Do() {
    if (!FindGeoAndTimeZone()) {
        return TResultValue();
    }

    TContext::TSlot* slotDate = Ctx.GetSlot("calendar_date");
    if (!CheckDateSlot(slotDate)) {
        return TResultValue();
    }

    TContext::TSlot* slotTense = Ctx.GetSlot("tense");
    if (!IsSlotEmpty(slotTense) && TStringBuf("past") == slotTense->Value.GetString()) {
        LookForward = false;
    }

    // [ADI-2] Always use client timestamp
    ui64 timeNow = Ctx.Meta().Epoch();
    NowTimePtr.reset(new TDateTime::TSplitTime(NDatetime::GetTimeZone(TimeZone), timeNow));

    NSc::TValue dateResult;

    if (!IsSlotEmpty(slotDate)) {
        InitAskedDate(slotDate->Value);
        FixAskedDate();
    }

    if (TResultValue error = CalculateOutputDate(&dateResult)) {
        Ctx.AddErrorBlock(error.GetRef());
        return TResultValue();
    }

    Ctx.CreateSlot(TStringBuf("resolved_date"), TStringBuf("time_result"), true, dateResult);

    NSc::TValue timeLocation;
    Geo.AddAllCaseForms(Ctx, &timeLocation);

    TString prepcase;
    Geo.GetNames(Ctx.MetaLocale().Lang, nullptr, &prepcase);
    if (prepcase) {
        timeLocation["city_prepcase"] = prepcase;
    }
    if (timeLocation.IsDict()) {
        Ctx.CreateSlot(TStringBuf("resolved_where"), TStringBuf("geo"), true, timeLocation);
    }
    MakeSuggest();
    if (Ctx.MetaClientInfo().IsSmartSpeaker()) {
        Ctx.AddStopListeningBlock();
    }
    return TResultValue();
}

} // end anon namespace

TResultValue TDateFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::GET_DATE);
    TDateRequestImpl impl(r);
    return impl.Do();
}

void TDateFormHandler::Register(THandlersMap* handlers) {
    auto cbDateForm = []() {
        return MakeHolder<TDateFormHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.get_date"), cbDateForm);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.get_date__ellipsis"), cbDateForm);
}

}
