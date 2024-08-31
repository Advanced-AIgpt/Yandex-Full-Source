#pragma once

#include "vins.h"

namespace NBASS {

class TFactsHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // NBASS
