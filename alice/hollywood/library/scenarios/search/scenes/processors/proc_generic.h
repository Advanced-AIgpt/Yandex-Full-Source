#pragma once

#include "processors.h"

namespace NAlice::NHollywoodFw::NSearch {

class TProcessorSearchGeneric : public ISearchProcessor {
public:
    TProcessorSearchGeneric()
        : ISearchProcessor("Generic")
    {
    }
    EProcessorRet ProcessSearchObject(const TRunRequest& runRequest,
                                      const TSearchResultParser& searchInfo,
                                      TSearchResult& results) const override;
};


} // namespace NAlice::NHollywoodFw::NSearch
