#pragma once

#include "vins.h"

namespace NBASS {

// Handler that does nothing, just returns form to VINS
class TNoOpFormHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace NBASS
