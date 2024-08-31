#pragma once

#include "table_helper.h"

#include <alice/matrix/library/logging/events/events.ev.pb.h>

namespace NMatrix::NNotificator::NAnalytics {

struct TAggregatedEventsInfo {
    TYtTimestamp MinTimestamp = Max<TYtTimestamp>();
    TYtTimestamp MaxTimestamp = 0;

    NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::EResult TechnicalPushValidationResult = NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::UNKNOWN;

    bool DeliveryAckRecieved = false;
};

using TAggregatedInfoPatcher = std::function<void(TAggregatedEventsInfo& aggregatedEventsInfo, const NYT::TTableReader<NYT::TNode>::TRowType& row)>;

extern const THashMap<TEventClass, TAggregatedInfoPatcher> AGGREGATED_INFO_PATCHERS;

} // namespace NMatrix::NNotificator::NAnalytics
