#pragma once

#include "navigator_intent.h"

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TNavigatorSimpleIntentsHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;
    static void Register(THandlersMap* handlers);
};

}
