#pragma once

#include "alice/bass/forms/vins.h"

namespace NBASS {

namespace NMarket {
class TMarketContext;
}

namespace NCrmbot {

class TCrmbotFormHandler : public IHandler {
public:
    using TMarketContext = NMarket::TMarketContext;

protected:
    void AddFeedbackAddon(TMarketContext& ctx) const;
};

}

}
