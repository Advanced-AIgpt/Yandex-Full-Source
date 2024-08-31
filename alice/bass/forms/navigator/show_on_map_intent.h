#pragma once

#include "navigator_intent.h"

#include <alice/bass/forms/geo_resolver.h>

namespace NBASS {

class TShowOnMapIntent : public INavigatorIntent {
public:
    TShowOnMapIntent(TContext& ctx, TGeoPosition pos, TStringBuf title = "")
        : INavigatorIntent(ctx, TStringBuf("show_point_on_map") /* scheme */)
        , Pos(pos)
        , Title(title)
    {}

    TString MakeIntentString();

private:
    TResultValue SetupSchemeAndParams() override;
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override;

private:
    const TGeoPosition Pos;
    const TString Title;
};

}
