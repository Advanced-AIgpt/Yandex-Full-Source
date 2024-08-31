#include "show_on_map_intent.h"

#include <alice/bass/forms/directives.h>

namespace NBASS {

TResultValue TShowOnMapIntent::SetupSchemeAndParams() {
    Params.InsertUnescaped(TStringBuf("lat"), ToString(Pos.Lat));
    Params.InsertUnescaped(TStringBuf("lon"), ToString(Pos.Lon));

    if (!Title.empty()) {
        Params.InsertUnescaped(TStringBuf("desc"), Title);
    }

    return TResultValue();
}

TDirectiveFactory::TDirectiveIndex TShowOnMapIntent::GetDirectiveIndex() {
    return GetAnalyticsTagIndex<TNavigatorShowPointOnMapDirective>();
}

TString TShowOnMapIntent::MakeIntentString() {
    SetupSchemeAndParams();
    return BuildIntentString();
}

}
