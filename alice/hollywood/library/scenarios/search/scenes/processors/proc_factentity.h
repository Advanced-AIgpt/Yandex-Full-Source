#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorFactEntity : public ISearchProcessor {
public:
    TProcessorFactEntity()
        : ISearchProcessor("Fact:Entity")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;
};

} // namespace NAlice::NHollywoodFw::NSearch
