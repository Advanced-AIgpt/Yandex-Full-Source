#include "table_helper.h"

#include <alice/matrix/notificator/analytics/common/yt_helpers.h>
#include <alice/matrix/notificator/analytics/common/column_names.h>

namespace NMatrix::NNotificator::NAnalytics {

NYT::TTableSchema GetMapperResultSchema() {
    return NYT::TTableSchema()
        .AddColumn(TString(PUSH_ID_COLUMN_NAME), NYT::EValueType::VT_STRING)
        .AddColumn(TString(TIMESTAMP_COLUMN_NAME), NYT::EValueType::VT_TIMESTAMP)
        .AddColumn(TString(PUSH_EVENT_TYPE_ID_COLUMN_NAME), NYT::EValueType::VT_UINT32)
        .AddColumn(TString(PUSH_EVENT_COLUMN_NAME), NYT::EValueType::VT_STRING);
}

NYT::TNode GetMapperResultRow(
    const TString& pushId,
    const TEventTimestamp timestamp,
    const TEventClass eventClass,
    const TString& eventProtoSerrialized
) {
    return NYT::TNode()
        (TString(PUSH_ID_COLUMN_NAME), pushId)
        (TString(TIMESTAMP_COLUMN_NAME), EventTimestampToYtTimestamp(timestamp))
        (TString(PUSH_EVENT_TYPE_ID_COLUMN_NAME), eventClass)
        (TString(PUSH_EVENT_COLUMN_NAME), eventProtoSerrialized);
}

} // namespace NMatrix::NNotificator::NAnalytics
