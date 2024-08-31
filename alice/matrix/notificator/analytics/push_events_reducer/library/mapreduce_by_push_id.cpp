#include "mapreduce_by_push_id.h"

#include "reducer.h"
#include "table_helper.h"

#include <alice/matrix/notificator/analytics/common/column_names.h>

namespace NMatrix::NNotificator::NAnalytics {

void ReducePushEventsById(
    NYT::IClientPtr client,
    const TVector<TString>& tables,
    const TString& outputTable
) {
    // TODO(ZION-257) define and construct schema somewhere else
    auto outputPath = NYT::TRichYPath(outputTable).Schema(GetReducerResultSchema());

    auto mapReduceSpec = NYT::TMapReduceOperationSpec();
    for (const auto& inputTable : tables) {
        mapReduceSpec = mapReduceSpec.AddInput<NYT::TNode>(inputTable);
    }
    mapReduceSpec = mapReduceSpec.AddOutput<NYT::TNode>(outputPath);
    mapReduceSpec = mapReduceSpec.ReduceBy(TString(NMatrix::NNotificator::NAnalytics::PUSH_ID_COLUMN_NAME));

    auto reduceOp = client->MapReduce(
        mapReduceSpec,
        nullptr,
        MakeIntrusive<NMatrix::NNotificator::NAnalytics::TPushEventsReducer>()
    );

    Cout << "Operation mapreduce: " << reduceOp->GetWebInterfaceUrl() << Endl;
}

} // namespace NMatrix::NNotificator::NAnalytics
