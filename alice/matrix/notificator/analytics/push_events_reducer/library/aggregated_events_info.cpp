#include "aggregated_events_info.h"

#include "enum_value_ordering.h"

#include <alice/matrix/notificator/analytics/common/column_names.h>

namespace NMatrix::NNotificator::NAnalytics {

template <typename TEventType>
TEventType ParseEventFromRow(const NYT::TTableReader<NYT::TNode>::TRowType& row) {
    auto eventProto = TEventType();
    eventProto.ParseFromStringOrThrow(row.ChildAsString(PUSH_EVENT_COLUMN_NAME));

    return eventProto;
}

void TechnicalPushValidationPatcher(TAggregatedEventsInfo& aggregatedEventsInfo, const NYT::TTableReader<NYT::TNode>::TRowType& row) {
    auto eventProto = ParseEventFromRow<NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult>(row);

    aggregatedEventsInfo.TechnicalPushValidationResult = NMatrix::NAnalytics::MaxEnumValueByAnalyticsPriority(
        aggregatedEventsInfo.TechnicalPushValidationResult,
        eventProto.GetResult()
    );
}

void TechnicalPushDeliveryAcknowledgePatcher(TAggregatedEventsInfo& aggregatedEventsInfo, const NYT::TTableReader<NYT::TNode>::TRowType& /* row */) {
    aggregatedEventsInfo.DeliveryAckRecieved = true;
}

const THashMap<TEventClass, TAggregatedInfoPatcher> AGGREGATED_INFO_PATCHERS = {
    {
        NEvClass::TMatrixNotificatorAnalyticsTechnicalPushValidationResult::ID,
        TechnicalPushValidationPatcher,
    },
    {
        NEvClass::TMatrixNotificatorAnalyticsTechnicalPushDeliveryAcknowledge::ID,
        TechnicalPushDeliveryAcknowledgePatcher,
    },
};

} // namespace NMatrix::NNotificator::NAnalytics
