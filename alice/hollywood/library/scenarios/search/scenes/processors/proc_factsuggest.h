#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorFactSuggest : public ISearchProcessor {
public:
    TProcessorFactSuggest()
        : ISearchProcessor("Fact:Suggest")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;

    // This function is shared with RICH facts
    static EProcessorRet Parse(const TRunRequest& runRequest,
                               const TSearchResultParser& searchInfo,
                               const google::protobuf::Struct& mainFact,
                               TSearchResult& results);
};

} // namespace NAlice::NHollywoodFw::NSearch
