#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS {

class TSingSongFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
