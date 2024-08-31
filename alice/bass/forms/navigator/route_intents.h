#pragma once

#include "navigator_intent.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geo_resolver.h>

namespace NBASS {

constexpr TStringBuf NAVIGATOR_ALICE_CONFIRMATION = "navigator_alice_confirmation";

class TBuildRouteNavigatorIntent : public INavigatorIntent {
public:
    TBuildRouteNavigatorIntent(TContext& ctx, const NSc::TValue& from, const NSc::TValue& via, const NSc::TValue& to);
    TResultValue Do();

private:
    TResultValue SetupSchemeAndParams() override;
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override;

    NHttpFetcher::THandle::TRef CreateRoutePointContextRequest(NSc::TValue& location);
    void RequestRoutePointContext();
    void AddPointParam(TStringBuf pointName, const NSc::TValue& point);

private:
    THashMap<TStringBuf, NSc::TValue> RoutePoints;
    NHttpFetcher::IMultiRequest::TRef MultiRequest;
};

class TShowGuidanceIntent : public ISchemeOnlyNavigatorIntent {
public:
    explicit TShowGuidanceIntent(TContext& ctx)
        : ISchemeOnlyNavigatorIntent(ctx, TStringBuf("show_ui/map/travel") /* scheme */)
    {}

protected:
    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorShowGuidanceDirective>();
    }
};

class TExternalConfirmationIntent : public INavigatorIntent {
public:
    TExternalConfirmationIntent(TContext& ctx, bool isConfirmed)
        : INavigatorIntent(ctx, TStringBuf("external_confirmation") /* scheme */)
        , IsConfirmed(isConfirmed)
    {}

protected:
    TResultValue SetupSchemeAndParams() override {
        Params.InsertUnescaped(TStringBuf("confirmed"), ToString(IsConfirmed));
        return TResultValue();
    }

    TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() override {
        return GetAnalyticsTagIndex<TNavigatorExternalConfirmationDirective>();
    }

    bool IsConfirmed;
};

}
