#pragma once

#include "vins.h"

namespace NBASS {

class TGamesOnboardingHandler : public IHandler {
public:
    static const TStringBuf GameOnboardingFormName;

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
