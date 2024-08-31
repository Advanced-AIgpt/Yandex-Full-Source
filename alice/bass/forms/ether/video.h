#pragma once

#include <alice/bass/forms/vins.h>

namespace NBASS::NEther {

class TVideoHandler : public IHandler {

public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};
}
