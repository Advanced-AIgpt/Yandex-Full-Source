#pragma once

#include "vins.h"

namespace NBASS { //TODO: add tests

class TWhatsNewHandler : public IHandler {
public:
    static const TStringBuf WhatsNewFormName;

    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
