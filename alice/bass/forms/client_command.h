#pragma once

#include "vins.h"

namespace NBASS {

class TClientCommandHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace NBASS
