#pragma once

#include "vins.h"

namespace NBASS {

// This handler will process not only currencies,
// but also units converter requests too. Sometime later...
class TConvertFormHandler: public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
