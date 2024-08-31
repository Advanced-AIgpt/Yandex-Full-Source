#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorFactOldFlow : public ISearchProcessor {
public:
    TProcessorFactOldFlow()
        : ISearchProcessor("Fact:OldFlow")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;
};

} // namespace NAlice::NHollywoodFw::NSearch
