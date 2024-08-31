#pragma once

#include "enums.h"

#include <alice/bass/forms/vins.h>

namespace NBASS {
namespace NExternalSkill {

class TSkillRecommendationInitializer {
public:
    static void Register(THandlersMap* handlers);
    static TContext::TPtr SetAsResponse(TContext& ctx, EServiceRequestCard requestCard);
};

} // namespace NBASS
} // namespace NExternalSkill

