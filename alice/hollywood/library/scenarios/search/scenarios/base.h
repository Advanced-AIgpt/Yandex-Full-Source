#pragma once

#include <alice/hollywood/library/scenarios/search/context/context.h>
#include <alice/hollywood/library/scenarios/search/utils/serp_helpers.h>

namespace NAlice::NHollywood::NSearch {

class TSearchScenario {
public:
    explicit TSearchScenario(TSearchContext& ctx);

    virtual bool Do(const TSearchResult& response);

    static bool AddRelated(TSearchContext& ctx, const TSearchResult& searchResult);
    static void PostProcessAnswer(TSearchContext& ctx, const TSearchResult& response);
protected:
    bool AddSerp(bool addSerpSuggest, bool autoAction = false, const TStringBuf query = {});

protected:
    TSearchContext& Ctx;
};

} // namespace NAlice::NHollywood
