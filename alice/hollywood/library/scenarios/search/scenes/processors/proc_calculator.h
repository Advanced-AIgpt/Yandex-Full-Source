#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorCalculator : public ISearchProcessor {
public:
    TProcessorCalculator()
        : ISearchProcessor("Fact:Calculator")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;
};

} // namespace NAlice::NHollywoodFw::NSearch
