#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorSearchPorno : public ISearchProcessor {
public:
    TProcessorSearchPorno()
        : ISearchProcessor("Porno")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;
};

} // namespace NAlice::NHollywoodFw::NSearch
