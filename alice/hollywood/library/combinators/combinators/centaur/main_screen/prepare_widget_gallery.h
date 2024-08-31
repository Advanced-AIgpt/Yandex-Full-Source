#pragma once

#include <alice/hollywood/library/combinators/combinators/centaur/widget_service.h>

namespace NAlice::NHollywood::NCombinators {

class TPrepareWidgetGallery {
public:
    TPrepareWidgetGallery(THwServiceContext& ctx, const NScenarios::TCombinatorRequest& combinatorRequest);
    void Do(const TSemanticFrame& semanticFrame);

private:
    NData::TCentaurWidgetCardData WidgetGalleryCard(const NData::TCentaurWidgetConfigData& widgetConfigData, const int column, const int row);

    THwServiceContext& Ctx;
    TCombinatorRequestWrapper Request;
    NScenarios::TScenarioRunResponse ResponseForRenderer;
    TCombinatorContextWrapper CombinatorContextWrapper;
    const TWidgetResponses WidgetResponses;
};

} // namespace NAlice::NHollywood::NCombinators
