#pragma once

#include "navigator_intent.h"
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/vins.h>


namespace NBASS {

class TRefuelNavigatorIntent : public INavigatorIntent {
public:
    TRefuelNavigatorIntent(TContext& ctx, TCgiParameters& params)
        : INavigatorIntent(ctx, TStringBuf("tanker") /* scheme */)
    {
        Params = params;
    }
protected:

    // INavigatorIntent overrides:
    TResultValue SetupSchemeAndParams() override {
        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorRefuelDirective>();
    }
};

class TNavigatorRefuelHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);
};

} // namespace
