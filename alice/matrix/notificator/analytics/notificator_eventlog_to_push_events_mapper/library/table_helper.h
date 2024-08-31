#include <mapreduce/yt/interface/operation.h>

#include <library/cpp/eventlog/eventlog.h>

namespace NMatrix::NNotificator::NAnalytics {

NYT::TTableSchema GetMapperResultSchema();

NYT::TNode GetMapperResultRow(
    const TString& pushId,
    const TEventTimestamp timestamp,
    const TEventClass eventClass,
    const TString& eventProtoSerrialized
);

} // namespace NMatrix::NNotificator::NAnalytics
