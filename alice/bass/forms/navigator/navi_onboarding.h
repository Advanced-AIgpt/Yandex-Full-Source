#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TNavigatorOnboardingHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);

private:
    TResultValue ResponseWithWhatCanYouDo(TContext& context);
    TResultValue ResponseWithSessionStart(TContext& context);
};

}
