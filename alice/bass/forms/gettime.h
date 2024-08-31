#pragma once

#include "vins.h"

namespace NBASS {

class TTimeFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
