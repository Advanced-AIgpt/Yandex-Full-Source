#include "number_filter_worker.h"
#include "client.h"
#include "forms.h"
#include "context.h"

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <regex>
#include <cmath>

namespace NBASS {

namespace NMarket {

TNumberFilterWorker::TNumberFilterWorker(TMarketContext& ctx)
    : Ctx(ctx)
{
}

void TNumberFilterWorker::GetAmountInterval(NSc::TValue& amountFrom, NSc::TValue& amountTo, bool needRange) const
{
    TMaybe<double> from, to;
    if (!amountFrom.IsNull()) {
        from = amountFrom.GetNumber();
    }
    if (!amountTo.IsNull()) {
        to = amountTo.GetNumber();
    }
    GetAmountInterval(from, to, needRange);
    amountFrom = from.Defined() ? NSc::TValue(from.GetRef()) : NSc::TValue();
    amountTo = to.Defined() ? NSc::TValue(to.GetRef()) : NSc::TValue();
}

void TNumberFilterWorker::GetAmountInterval(
    TMaybe<double>& amountFrom,
    TMaybe<double>& amountTo,
    bool needRange) const
{
    static const double DEFAULT_AMOUNT_INTERVAL = 0.1;
    if (Ctx.DoesAmountExist()) {
        double amount = Ctx.GetAmount();
        if (needRange) {
            amountFrom = amount * (1 - DEFAULT_AMOUNT_INTERVAL);
            amountTo = amount * (1 + DEFAULT_AMOUNT_INTERVAL);
        } else {
            amountFrom = amount;
            amountTo = amount;
        }
    } else {
        if (Ctx.DoesAmountFromExist() && Ctx.DoesAmountToExist()) {
            double valueFrom = Ctx.GetAmountFrom();
            double valueTo = Ctx.GetAmountTo();
            if (valueFrom < 1000 && (std::lround(valueTo) % 1000 == 0) && (valueFrom * 1000 < valueTo)) {
                valueFrom *= 1000;
            } else if (valueTo < 1000 && (std::lround(valueFrom) % 1000 == 0) && (valueTo * 1000 > valueFrom)) {
                valueTo *= 1000;
            }
            amountFrom = valueFrom;
            amountTo = valueTo;
        } else {
            if (Ctx.DoesAmountFromExist()) {
                amountFrom = Ctx.GetAmountFrom();
                if (amountTo.Defined() && amountTo.GetRef() < amountFrom.GetRef()) {
                    LOG(DEBUG) << "Reset amountTo value " << amountTo
                               << " because it doesn't fit new amountFrom value " << amountFrom
                               << Endl;
                    amountTo.Clear();
                }
            }
            if (Ctx.DoesAmountToExist()) {
                amountTo = Ctx.GetAmountTo();
                if (amountFrom.Defined() && amountFrom.GetRef() > amountTo.GetRef()) {
                    LOG(DEBUG) << "Reset amountFrom value " << amountFrom
                               << " because it doesn't fit new amountTo value " << amountTo
                               << Endl;
                    amountFrom.Clear();
                }
            }
        }
    }
    LOG(DEBUG)
        << "Got amount interval: "
        << (amountFrom ? ToString(amountFrom.GetRef()) : "<null>") << " - "
        << (amountTo ? ToString(amountTo.GetRef()) : "<null>") << Endl;
}

void TNumberFilterWorker::UpdateFilter(TStringBuf filterId)
{
    const auto& filters = Ctx.GetGlFilters();
    TMaybe<double> from, to;
    if (filters.contains(filterId)) {
        const auto& filter = filters.at(filterId);
        if (std::holds_alternative<TNumberGlFilter>(filter)) {
            const auto& numberFilter = std::get<TNumberGlFilter>(filter);
            from = numberFilter.Min;
            to = numberFilter.Max;
        } else if (!std::holds_alternative<TRawGlFilter>(filter)) {
            LOG(ERR) << "Expected number gl filter with id \"" << filterId << "\" but got other type" << Endl;
            Y_ASSERT(false);
        }
    }
    GetAmountInterval(from, to, Ctx.DoesAmountNeedRange());

    Ctx.AddGlFilter(TNumberGlFilter(filterId, from, to));
}

} // namespace NMarket

} // namespace NBASS
