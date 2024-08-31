#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TGetMyLocationHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

private:
    bool HasActualLocation(const TContext& ctx) const;
    TStringBuf GetLocationZoom(const TContext& ctx) const;
    void ProcessLocation(TContext& ctx, const NSc::TValue& location);
    void ProcessError(TContext& ctx, TError::EType errorType, const TStringBuf& errorMessage);
    void AddSuggests(TContext& ctx);
};

}
