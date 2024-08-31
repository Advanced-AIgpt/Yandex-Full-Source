#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorFactRich : public ISearchProcessor {
public:
    TProcessorFactRich()
        : ISearchProcessor("Fact:Rich")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;
};

} // namespace NAlice::NHollywoodFw::NSearch
