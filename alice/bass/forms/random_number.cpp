#include "random_number.h"

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

void TRandomNumberFormHandler::Register(THandlersMap* handlers) {
    handlers->emplace(TStringBuf("personal_assistant.scenarios.random_num"), []() { return MakeHolder<TRandomNumberFormHandler>(); });
}

TResultValue TRandomNumberFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::RANDOM_NUMBER);

    TContext::TSlot* slotFrom = r.Ctx().GetOrCreateSlot(TStringBuf("count_from"), TStringBuf("num"));
    if (slotFrom->Value.IsNull()) {
        slotFrom->Value.SetIntNumber(1);
    }

    TContext::TSlot* slotTo = r.Ctx().GetOrCreateSlot(TStringBuf("count_to"), TStringBuf("num"));
    if (slotTo->Value.IsNull()) {
        slotTo->Value.SetIntNumber(100);
    }

    i64 from = slotFrom->Value.GetIntNumber();
    i64 to = slotTo->Value.GetIntNumber();
    if (to < from) {
        std::swap(to, from);
    }

    r.Ctx().CreateSlot(
        TStringBuf("result"),
        TStringBuf("num"),
        true,
        r.Ctx().GetRng().RandomInteger(from, to + 1)
    );

    return TResultValue();
}

}
