#include "base_handler.h"

#include <alice/bass/forms/market/context.h>

namespace NBASS::NCrmbot {

void TCrmbotFormHandler::AddFeedbackAddon(NMarket::TMarketContext& ctx) const {
    ctx.AddTextCardBlock("feedback_addon");
    NSc::TValue nobr{}, br{};
    nobr["nobr"] = true;
    br["nobr"] = false;
    ctx.AddSuggest("feedback_yes", br);
    ctx.AddSuggest("feedback_no", nobr);
}

}

