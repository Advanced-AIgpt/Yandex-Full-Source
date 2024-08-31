#pragma once

#include "vins.h"

namespace NBASS {

class TEtherHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
