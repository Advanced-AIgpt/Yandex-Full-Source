#pragma once

#include "vins.h"

namespace NBASS {

// Form for onboarding form that is used for first session starts and
// if user asks what you can do. Form has hidden slot for keeping onboarding
// set number used last time.
class TOnboardingHandler : public IHandler {
public:
    static const TStringBuf OnboardingFormName;

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
    static TContext::TPtr SetAsResponse(TContext& ctx);
};

class TOnboardingCancelHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

class TOnboardingNextHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
