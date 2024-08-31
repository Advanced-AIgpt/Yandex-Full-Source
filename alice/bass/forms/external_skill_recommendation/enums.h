#pragma once

#include <alice/library/onboarding/enums.h>

namespace NBASS::NExternalSkill {

using ESkillRequirement = NAlice::NOnboarding::ESkillRequirement;

enum class ESkillRecommendationExperiment {
    ServiceExperimentNamePrefix /* "skill_recommendation_service_experiment_name_" */,
    SkillRecommendation /* "skill_recommendation_experiment" */,
    SkillRecommendationFirstSession /* "skill_recommendation_first_session_experiment" */,
    OnboardingUrl /* "skill_recommendation_onboarding_url_experiment" */
};

enum class EStatsIncConuter {
    SkillRecommendation /* "skill_recommendation" */,
    SkillRecommendationCard /* "skill_recommendation_card_" */,
    ServiceError /* "skill_recommendation_service_error" */,
    SkillRecommendationZeroResponse /* "skill_recommendation_returned_zero" */
};

enum class EServiceRequestCard {
    Onboarding /* "onboarding" */,
    GamesOnboarding /* "games_onboarding" */,
    GetGreetings /* "get_greetings" */,
    FirstSessionOnboarding /* "first_session_onboarding" */,
    WhatsNew /* "whats_new" */,
    DiscoveryMegamindGC /* discovery_megamind_gc */,
    DiscoveryBASSSearch /* "discovery_bass_search" */,
    DiscoveryBASSUnknown /* "discovery_bass_unknown" */
};

} // namespace NBASS::NExternalSkill
