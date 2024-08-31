#include "reducer.h"

#include "aggregated_events_info.h"

#include "enum_value_ordering.h"
#include "table_helper.h"

#include <alice/matrix/notificator/analytics/common/column_names.h>
#include <alice/matrix/notificator/analytics/common/yt_helpers.h>

#include <alice/matrix/library/logging/events/events.ev.pb.h>

#include <library/cpp/eventlog/eventlog.h>

#include <util/string/vector.h>

namespace NMatrix::NNotificator::NAnalytics {

namespace {

void UpdateAggregatedEventsInfo(
    TAggregatedEventsInfo& aggregatedEventsInfo,
    const NYT::TTableReader<NYT::TNode>::TRowType& row
) {
    auto timestamp = row.ChildAsUint64(TIMESTAMP_COLUMN_NAME);
    aggregatedEventsInfo.MinTimestamp = Min(aggregatedEventsInfo.MinTimestamp, timestamp);
    aggregatedEventsInfo.MaxTimestamp = Max(aggregatedEventsInfo.MaxTimestamp, timestamp);

    auto eventId = row.ChildAsUint64(PUSH_EVENT_TYPE_ID_COLUMN_NAME);
    if (const auto* patcher = AGGREGATED_INFO_PATCHERS.FindPtr(eventId)) {
        (*patcher)(aggregatedEventsInfo, row);
    }
}

bool DeduceDelivered(const TAggregatedEventsInfo& aggregatedEventsInfo) {
    return aggregatedEventsInfo.DeliveryAckRecieved;
}

} // namespace

void TPushEventsReducer::Do(TReader* reader, TWriter* writer) {
    THashMap<TString, TAggregatedEventsInfo> eventsByPushId;

    for (const auto& cursor : *reader) {
        auto row = cursor.GetRow();
        const auto& eventName = row.ChildAsString(PUSH_ID_COLUMN_NAME);

        TAggregatedEventsInfo& aggrInfo = eventsByPushId[eventName];

        UpdateAggregatedEventsInfo(aggrInfo, row);
    }

    for (const auto& [pushId, aggrInfo] : eventsByPushId) {
        writer->AddRow(
            GetReducerResultRow(
                pushId,
                aggrInfo.MinTimestamp,
                aggrInfo.MaxTimestamp,
                aggrInfo.TechnicalPushValidationResult,
                DeduceDelivered(aggrInfo)
            )
        );
    }
}

REGISTER_REDUCER(TPushEventsReducer);

} // namespace NMatrix::NNotificator::NAnalytics
