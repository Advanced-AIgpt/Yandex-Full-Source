#include "browser_read_page.h"

#include <alice/bass/forms/directives.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

void TBrowserReadPageFormHandler::Register(THandlersMap* handlers) {
    auto handler = []() { return MakeHolder<TBrowserReadPageFormHandler>(); };
    handlers->RegisterFormHandler(TStringBuf("personal_assistant.scenarios.browser_read_page"), handler);
    handlers->RegisterFormHandler(TStringBuf("personal_assistant.scenarios.browser_read_page_pause"), handler);
    handlers->RegisterFormHandler(TStringBuf("personal_assistant.scenarios.browser_read_page_continue"), handler);
}

TResultValue TBrowserReadPageFormHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    TStringBuf formName = ctx.FormName();
    TStringBuf command;

    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::COMMANDS_OTHER);

    if (formName.AfterPrefix(TStringBuf("personal_assistant.scenarios."), command)) {
        if (command == TStringBuf("browser_read_page")) {
            ctx.AddCommand<TBrowserReadPageDirective>(TStringBuf("read_page"), NSc::TValue());
            return TResultValue();
        } else if (command == TStringBuf("browser_read_page_pause")) {
            ctx.AddCommand<TBrowserReadPagePauseDirective>(TStringBuf("read_page_pause"), NSc::TValue());
            return TResultValue();
        } else if (command == TStringBuf("browser_read_page_continue")) {
            ctx.AddCommand<TBrowserReadPageContinueDirective>(TStringBuf("read_page_continue"), NSc::TValue());
            return TResultValue();
        }
    }
    return TError(TError::EType::NOTSUPPORTED, TStringBuf("nonsupported form name"));
}

}
