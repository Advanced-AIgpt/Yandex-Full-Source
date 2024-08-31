#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/widget_service.h>

namespace NAlice::NHollywood::NCombinators {

class TAddWidgetFromGallery {
public:
    TAddWidgetFromGallery(THwServiceContext& ctx, const NScenarios::TCombinatorRequest& combinatorRequest);
    void Do(const TSemanticFrame& semanticFrame);

private:
    NScenarios::TDirective PrepareCollectMainScreenCallbackDirective();
    NScenarios::TDirective PrepareCollectMainScreenWithWidgetConfigCallbackDirective(
        const ::NAlice::NData::TCentaurWidgetConfigData& widgetConfigData, int column, int row);

    THwServiceContext& Ctx;
    TCombinatorRequestWrapper Request;
    NScenarios::TScenarioRunResponse ResponseForRenderer;
    TCombinatorContextWrapper CombinatorContextWrapper;
};

} // namespace NAlice::NHollywood::NCombinators
