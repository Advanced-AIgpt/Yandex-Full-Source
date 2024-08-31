/*
    FACTOID SEARCH PROCESSOR

    Rich facts processing
*/

#include "proc_factrich.h"
#include "proc_factsuggest.h"


#include <alice/protos/data/scenario/search/fact.pb.h>

namespace NAlice::NHollywoodFw::NSearch {

namespace {

constexpr TStringBuf SNIPPET_TYPE = "rich_fact";

} // anonymous namespace

ISearchProcessor::EProcessorRet TProcessorFactRich::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo, TSearchResult& results) const
{
    auto& logger = runRequest.Debug().Logger();
    // Try to extract factiod for
    const auto mainFact = searchInfo.FindFactoidByType(SNIPPET_TYPE);
    if (!mainFact) {
        LOG_DEBUG(logger) << "`rich_fact` fact is not found, processor is not suitable";
        return ISearchProcessor::EProcessorRet::Unknown;
    }
    return TProcessorFactSuggest::Parse(runRequest, searchInfo, *mainFact, results);
}

} // namespace NAlice::NHollywoodFw::NSearch
