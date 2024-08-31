#pragma once

#include "handler.h"

namespace NBASS::NPushNotification {

static const TString ONBOARDING_EVENT = "onboarding_store";

class TOnboardingPush : public IHandlerGenerator {
public:
    TResultValue Generate(THandler& holder, TApiSchemeHolder scheme) final override;
};

} // namespace NBASS::NPushNotification
