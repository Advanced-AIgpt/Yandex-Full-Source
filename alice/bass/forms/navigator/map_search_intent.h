#pragma once

#include "navigator_intent.h"

#include <alice/bass/forms/geo_resolver.h>

namespace NBASS {

class TMapSearchNavigatorIntent : public INavigatorIntent {
public:
    TMapSearchNavigatorIntent(TContext& ctx, TString searchText, TMaybe<TGeoPosition> searchPos)
        : INavigatorIntent(ctx, TStringBuf("map_search") /* scheme */)
        , SearchText(searchText)
        , SearchPos(searchPos)
    {}

    TString MakeIntentString();

private:
    TResultValue SetupSchemeAndParams() override;
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override;

private:
    TString SearchText;
    TMaybe<TGeoPosition> SearchPos;
};

}
