#include "unit_test_form.h"

namespace NBASS {

const TStringBuf TUnitTestFormHandler::DEFAULT_FORM_NAME = "unit_test_form_handler.default";

TResultValue TUnitTestFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    const TSlot* slotAnswer = ctx.GetSlot("unit_test_answer");

    if (IsSlotEmpty(slotAnswer)) {
        return Nothing();
    }

    if (const NSc::TArray& slotsJson = slotAnswer->Value["slots"].GetArray()) {
        for (const NSc::TValue& slotJson : slotsJson) {
            ctx.CreateSlot(slotJson["name"].GetString(), slotJson["type"].GetString(), slotJson["optional"].GetBool(), slotJson["value"], slotJson["source_text"]);
        }
    }

    return Nothing();
}

void TUnitTestFormHandler::Register(THandlersMap* handlers) {
    handlers->RegisterFormHandler(DEFAULT_FORM_NAME, []() { return MakeHolder<TUnitTestFormHandler>(); });
}

} // namespace NBASS
