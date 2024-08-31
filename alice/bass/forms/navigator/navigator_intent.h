#pragma once

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/libs/analytics/analytics.h>

namespace NBASS {
class INavigatorIntent {
public:
    explicit INavigatorIntent(TContext& ctx, TStringBuf scheme = TStringBuf(""))
        : Context(ctx)
        , Scheme(scheme)
    {}

    TResultValue Do();

    virtual ~INavigatorIntent() = default;

protected:
    virtual TResultValue SetupSchemeAndParams() = 0;
    virtual TDirectiveFactory::TDirectiveIndex GetDirectiveIndex() = 0;

    TString BuildIntentString() const;

    void AddShortAnswerBlock();

protected:
    TContext& Context;

    TString Scheme;
    TCgiParameters Params;
    bool ListeningIsPossible = false;
};

class ISchemeOnlyNavigatorIntent : public INavigatorIntent {
public:
    ISchemeOnlyNavigatorIntent(TContext& ctx, TStringBuf scheme)
        : INavigatorIntent(ctx, scheme)
    {}

protected:
    TResultValue SetupSchemeAndParams() override {
        return {};
    }
};

}
