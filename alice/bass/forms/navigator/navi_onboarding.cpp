#include "navi_onboarding.h"

#include <alice/bass/forms/session_start.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

static constexpr TStringBuf NAVI_ONBOARDING = "personal_assistant.navi.onboarding";
static constexpr TStringBuf NAVI_ONBOARDING_SKIP = "personal_assistant.navi.onboarding__skip";
static constexpr TStringBuf NAVI_ONBOARDING_NEXT = "personal_assistant.navi.onboarding__next";

TResultValue TNavigatorOnboardingHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::NAVI_COMMANDS);

    TContext::TSlot* phraseNumSlot = r.Ctx().GetOrCreateSlot("phrase_number", "num");
    if (r.Ctx().FormName() == NAVI_ONBOARDING) {
        phraseNumSlot->Value.SetIntNumber(0);
    } else if (r.Ctx().FormName() == NAVI_ONBOARDING_NEXT) {
        return ResponseWithWhatCanYouDo(r.Ctx());
    } else if (r.Ctx().FormName() == NAVI_ONBOARDING_SKIP) {
        return ResponseWithSessionStart(r.Ctx());
    }

    r.Ctx().AddSuggest("onboarding__next");
    r.Ctx().AddSuggest("onboarding__skip");

    return TResultValue();
}

void TNavigatorOnboardingHandler::Register(THandlersMap* handlers) {
    auto NavigatorOnboardingForm = []() {
        return MakeHolder<TNavigatorOnboardingHandler>();
    };

    handlers->emplace(NAVI_ONBOARDING, NavigatorOnboardingForm);
    handlers->emplace(NAVI_ONBOARDING_SKIP, NavigatorOnboardingForm);
    handlers->emplace(NAVI_ONBOARDING_NEXT, NavigatorOnboardingForm);
}

TResultValue TNavigatorOnboardingHandler::ResponseWithWhatCanYouDo(TContext& context) {
    TContext::TPtr newContext;
    try {
        newContext = TSessionStartFormHandler::SetAsResponse(context);
    } catch (const yexception& e) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Failed switching to what_can_you_do from navi_onboarding: " << e.what());
    }

    return context.RunResponseFormHandler();
}

TResultValue TNavigatorOnboardingHandler::ResponseWithSessionStart(TContext& context) {
    TContext::TPtr newContext;
    try {
        newContext = TSessionStartFormHandler::SetSessionStartAsResponse(context);
    } catch (const yexception& e) {
        return TError(TError::EType::SYSTEM,
                      TStringBuilder() << "Failed switching to start_session from navi_onboarding: " << e.what());
    }

    TContext::TSlot* modeSlot = newContext->GetOrCreateSlot("mode", "string");
    modeSlot->Value.SetString("greeting");

    return context.RunResponseFormHandler();
}

}
