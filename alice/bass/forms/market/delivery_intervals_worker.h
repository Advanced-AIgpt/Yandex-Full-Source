#pragma once

#include "client.h"
#include "context.h"

#include <alice/bass/forms/market/client/bool_scheme_traits.h>
#include <alice/bass/forms/market/client/checkout.sc.h>

#include <alice/library/datetime/datetime.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <utility>

namespace NBASS {

namespace NMarket {

struct TDeliveryIntervalsContext {
    TDeliveryIntervalsContext(
            const TVector<bool>& filteredDeliveryOptions,
            const NAlice::TDateTime::TSplitTime& leftBound,
            NAlice::TDateTime userTime)
        : FilteredDeliveryOptions(filteredDeliveryOptions)
        , LeftBound(leftBound)
        , UserTime(std::move(userTime))
    {
    }

    TVector<bool> FilteredDeliveryOptions;
    NAlice::TDateTime::TSplitTime LeftBound;
    NAlice::TDateTime UserTime;
};

class TDeliveryIntervalsWorker {
public:
    using TState = NBassApi::TMarketCheckoutState<TBoolSchemeTraits>;

    explicit TDeliveryIntervalsWorker(TMarketContext& ctx);

    TMaybe<i64> Handle(TState& state);

private:
    NAlice::TDateTime GetDate(const NSc::TValue& date, const NAlice::TDateTime& userDateTime, const NAlice::TDateTime::TSplitTime& leftBound);

    void FilterOptionsByDay(const TState& state, TDeliveryIntervalsContext& context);

    void FilterOptionsByDateTime(
        const TState& state,
        TContext::TSlot* dateTimeSlot,
        TDeliveryIntervalsContext& context,
        bool compareFromDateTime
    );

    NAlice::TDateTime::TSplitTime ParseSplitTime(TStringBuf date, TStringBuf time);

    NAlice::TDateTime::TSplitTime GetDeliveryLeftBound(const TState& state);

    bool CompareDeliveryDateTimes(
        const NAlice::TDateTime::TSplitTime& checkouterDateTime,
        const NAlice::TDateTime& userDateTime,
        const NSc::TValue& dateTimeSlot
    );

    bool CompareDeliveryDates(
        const NAlice::TDateTime::TSplitTime& checkouterDateTime,
        const NAlice::TDateTime& userDate,
        const NSc::TValue& dateTimeSlot
    );

private:
    TMarketContext& Ctx;
};

}

}
