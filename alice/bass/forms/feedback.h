#pragma once

#include "vins.h"

namespace NBASS {

// Just simple feedback suggests (for positive and negative)
class TFeedbackFormHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& r) override;

    static void Register(THandlersMap* handlers);
};

}
