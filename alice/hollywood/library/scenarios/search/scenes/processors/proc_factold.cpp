/*
    FACTOID SEARCH PROCESSOR

    All non-hanldled factiod objects should be prepared with old scenario

    This is a temporary processor, it will be removed when all factoid answers will be  moved to HWF
*/

#include "proc_factold.h"

namespace NAlice::NHollywoodFw::NSearch {

ISearchProcessor::EProcessorRet TProcessorFactOldFlow::ProcessSearchObject(const TRunRequest& runRequest,
    const TSearchResultParser& searchInfo,
    TSearchResult& results) const
{
    Y_UNUSED(runRequest);
    Y_UNUSED(results);
    if (searchInfo.HasFactoid()) {
        // Data has factoid answer and this fact was not parsed by existing processors - switch immediately to old flow
        return ISearchProcessor::EProcessorRet::OldFlow;
    }
    return ISearchProcessor::EProcessorRet::Unknown;
}

} // namespace NAlice::NHollywoodFw::NSearch
