#include "no_op.h"

#include <alice/bass/libs/config/config.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace {

constexpr TStringBuf CANCEL_LIST_FORM_NAME = "personal_assistant.scenarios.common.cancel_list";
constexpr TStringBuf DO_NOT_UNDERSTAND_FORM_NAME = "personal_assistant.scenarios.do_not_understand";
constexpr TStringBuf TEACH_ME_FORM_NAME = "personal_assistant.scenarios.teach_me";

} // namespace

namespace NBASS {

TResultValue TNoOpFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::PLACEHOLDERS);
    return TResultValue();
}

void TNoOpFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
       return MakeHolder<TNoOpFormHandler>();
    };

    handlers->emplace(CANCEL_LIST_FORM_NAME, handler);
    handlers->emplace(DO_NOT_UNDERSTAND_FORM_NAME, handler);
    handlers->emplace(TEACH_ME_FORM_NAME, handler);
}

}
