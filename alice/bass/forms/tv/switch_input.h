#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TTvSwitchInputFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

} // namespace NBASS
