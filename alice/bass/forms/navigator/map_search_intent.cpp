#include "map_search_intent.h"

#include <alice/bass/forms/directives.h>

namespace NBASS {

TResultValue TMapSearchNavigatorIntent::SetupSchemeAndParams() {
    Params.InsertUnescaped(TStringBuf("text"), SearchText);
    if (SearchPos) {
        Params.InsertUnescaped(TStringBuf("ll"), SearchPos->GetLonLatString());
    }

    AddShortAnswerBlock();

    return TResultValue();
}

TDirectiveFactory::TDirectiveIndex TMapSearchNavigatorIntent::GetDirectiveIndex() {
    return GetAnalyticsTagIndex<TNavigatorMapSearchDirective>();
}

TString TMapSearchNavigatorIntent::MakeIntentString() {
    SetupSchemeAndParams();
    return BuildIntentString();
}

}
