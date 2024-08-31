#include "table_helper.h"

#include <alice/matrix/notificator/analytics/common/yt_helpers.h>
#include <alice/matrix/notificator/analytics/common/column_names.h>

namespace NMatrix::NNotificator::NAnalytics {

NYT::TTableSchema GetReducerResultSchema() {
    return NYT::TTableSchema()
        .AddColumn(TString(PUSH_ID_COLUMN_NAME), NYT::EValueType::VT_STRING)
        .AddColumn(TString(MIN_TIMESTAMP_COLUMN_NAME), NYT::EValueType::VT_TIMESTAMP)
        .AddColumn(TString(MAX_TIMESTAMP_COLUMN_NAME), NYT::EValueType::VT_TIMESTAMP)
        .AddColumn(TString(VALIDATION_RESULT_COLUNM_NAME), NYT::EValueType::VT_UINT32)
        .AddColumn(TString(DELIVERED_COLUMN_NAME), NYT::EValueType::VT_BOOLEAN);
}

NYT::TNode GetReducerResultRow(
    const TString& pushId,
    const TYtTimestamp minTimestamp,
    const TYtTimestamp maxTimestamp,
    const ui32 validationResult,
    const bool delivered
) {
    return NYT::TNode()
        (TString(PUSH_ID_COLUMN_NAME), pushId)
        (TString(MIN_TIMESTAMP_COLUMN_NAME), minTimestamp)
        (TString(MAX_TIMESTAMP_COLUMN_NAME), maxTimestamp)
        (TString(VALIDATION_RESULT_COLUNM_NAME), validationResult)
        (TString(DELIVERED_COLUMN_NAME), delivered);
}

} // namespace NMatrix::NNotificator::NAnalytics
