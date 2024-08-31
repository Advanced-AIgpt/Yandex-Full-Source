#include "search_callback_directive_model.h"

#include <alice/megamind/library/common/defs.h>

namespace NAlice::NMegamind {

namespace {

TTypedSemanticFrame GetSearchTypedSemanticFrame(const TString& query) {
    TTypedSemanticFrame typedSemanticFrame;
    typedSemanticFrame.MutableSearchSemanticFrame()->MutableQuery()->SetStringValue(query);
    return typedSemanticFrame;
}

TAnalyticsTrackingModule GetSearchAnalytics() {
    TAnalyticsTrackingModule analytics;
    analytics.SetOrigin(TAnalyticsTrackingModule::Scenario);
    analytics.SetPurpose("search");

    return analytics;
}

} // namespace

TSearchCallbackDirectiveModel::TSearchCallbackDirectiveModel(const TString& query)
    : TTypedSemanticFrameRequestDirectiveModel(GetSearchTypedSemanticFrame(query),
                                               GetSearchAnalytics(),
                                               /* params= */ Nothing(),
                                               /* requestParams= */ Nothing(),
                                               query) {
}

} // namespace NAlice::NMegamind
