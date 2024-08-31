#pragma once

#include <alice/bass/forms/external_skill_recommendation/service_response.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

namespace NBASS::NExternalSkill {

using TServiceResponseRef = NBASSSkill::TServiceResponse<TSchemeTraits>;
using TServiceResponse = TSchemeHolder<TServiceResponseRef::TConst>;

}

