#pragma once

#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/push.h>
#include <alice/hollywood/library/response/response_builder.h>

#include <alice/library/logger/logger.h>
#include <alice/library/util/rng.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>

#include <library/cpp/json/writer/json_value.h>

#include <util/generic/string.h>

namespace NAlice::NHollywood::NMusic {

bool ChildAgeIsSet(const NData::TChildAge& age);

bool ShouldAddChildAgePromo(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    IRng& rng);

void AddChildAgePromoAttention(NJson::TJsonValue& stateJson);

void AddChildAgePromoDirectives(
    TRTLogger& logger,
    const TScenarioApplyRequestWrapper& applyRequest,
    TResponseBodyBuilder& bodyBuilder);

} // namespace NAlice::NHollywood::NMusic
