#include "old_flow.h"

#include <alice/hollywood/library/scenarios/search/scenes/processors/processors.h>

#include <alice/megamind/protos/analytics/scenarios/search/search.pb.h>
#include <alice/library/search_result_parser/search_result_parser.h>

namespace NAlice::NHollywoodFw::NSearch {

TRetMain TSearchOldFlowScene::Main(const NHollywood::TSearchEmptyProto&, const TRunRequest& request, TStorage&, const TSource&) const {
    // Prepare AI for old scenes
    TSearchResultParser searchResults(request.Debug().Logger());
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS, false));
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_DOCS_RIGHT, false));
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZPLACES, false));
    searchResults.AttachDataSource(request.GetDataSource(EDataSourceType::WEB_SEARCH_WIZARD, false));

    auto aiObject = TSearchProcessorInstance::CreateAiLog(searchResults);
    request.AI().AddObject(std::move(aiObject));
    // This AI info will be merged in hook callback
    return TReturnValueDo();
}

} // namespace NAlice::NHollywoodFw::NSearch
