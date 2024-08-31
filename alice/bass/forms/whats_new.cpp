#include "whats_new.h"

#include "external_skill_recommendation/skill_recommendation.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/metrics/metrics.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

const TStringBuf TWhatsNewHandler::WhatsNewFormName = TStringBuf("personal_assistant.scenarios.whats_new");

TResultValue TWhatsNewHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::ONBOARDING);
    if (NExternalSkill::TSkillRecommendationInitializer::SetAsResponse(r.Ctx(), NExternalSkill::EServiceRequestCard::WhatsNew)) {
        return r.Ctx().RunResponseFormHandler();
    }
    Y_STATS_INC_COUNTER("bass_skill_recommendation_service_error");
    return TError{TError::EType::SKILLSERVICEERROR, "Whats new: Skill recommendation service failure"};
}

void TWhatsNewHandler::Register(THandlersMap* handlers) {
    auto handler = []() {
        return MakeHolder<TWhatsNewHandler>();
    };
    handlers->emplace(WhatsNewFormName, handler);
}
}
