#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/widget_service.h>

namespace NAlice::NHollywood::NCombinators {

class TPrepareDivPatch {
public:
    TPrepareDivPatch(THwServiceContext& ctx, const NScenarios::TCombinatorRequest& combinatorRequest);
    void Do(const TSemanticFrame& semanticFrame);

private:
    THwServiceContext& Ctx;
    TCombinatorRequestWrapper Request;
    NScenarios::TScenarioRunResponse ResponseForRenderer;
    TCombinatorContextWrapper CombinatorContextWrapper;
    const TWidgetResponses WidgetResponses;
};

} // namespace NAlice::NHollywood::NCombinators
