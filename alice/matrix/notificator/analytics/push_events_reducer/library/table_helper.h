#include <alice/matrix/notificator/analytics/common/yt_helpers.h>

#include <mapreduce/yt/interface/operation.h>

#include <library/cpp/eventlog/eventlog.h>

namespace NMatrix::NNotificator::NAnalytics {

NYT::TTableSchema GetReducerResultSchema();

NYT::TNode GetReducerResultRow(
    const TString& pushId,
    const TYtTimestamp minTimestamp,
    const TYtTimestamp maxTimestamp,
    const ui32 validationResult,
    const bool delivered
);

} // namespace NMatrix::NNotificator::NAnalytics
