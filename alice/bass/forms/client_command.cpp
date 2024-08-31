#include "client_command.h"
#include "directives.h"

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

const THashMap<TStringBuf, TDirectiveFactory::TDirectiveIndex> ALL_CLIENT_COMMANDS = {
    { TStringBuf("go_forward"), GetAnalyticsTagIndex<TClientGoForwardDirective>() },
    { TStringBuf("go_backward"), GetAnalyticsTagIndex<TClientGoBackwardDirective>() },
    { TStringBuf("go_up"), GetAnalyticsTagIndex<TClientGoUpDirective>() },
    { TStringBuf("go_down"), GetAnalyticsTagIndex<TClientGoDownDirective>() },
    { TStringBuf("go_top"), GetAnalyticsTagIndex<TClientGoTopDirective>() },
    { TStringBuf("go_home"), GetAnalyticsTagIndex<TClientGoHomeDirective>() },
    { TStringBuf("go_to_the_beginning"), GetAnalyticsTagIndex<TClientGoToBeginningDirective>() },
    { TStringBuf("go_to_the_end"), GetAnalyticsTagIndex<TClientGoToEndDirective>() },
};

TResultValue TClientCommandHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::COMMANDS_OTHER);
    TStringBuf formName = r.Ctx().FormName();
    TStringBuf commandName = formName.RNextTok('.');

    const auto it = ALL_CLIENT_COMMANDS.find(commandName);
    if (it != ALL_CLIENT_COMMANDS.end()) {
        r.Ctx().AddCommand(commandName, it->second, NSc::TValue::Null());
    } else {
        r.Ctx().AddCommand<TClientUnknownCommandDirective>(commandName, NSc::TValue::Null());
    }

    return TResultValue();
}

void TClientCommandHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TClientCommandHandler>();
    };
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_forward"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_backward"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_up"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_down"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_top"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_home"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_to_the_beginning"), handler);
    handlers->emplace(TStringBuf("personal_assistant.scenarios.quasar.go_to_the_end"), handler);
}

} // namespace NBASS
