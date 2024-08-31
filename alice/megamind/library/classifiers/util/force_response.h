#pragma once

#include <alice/megamind/library/context/context.h>

namespace NAlice {

bool ShouldForceResponseForEmptyRequestedSlot(const IContext& ctx, const TString& name);
bool ShouldForceResponseForFilledUntypedRequestedSlot(const IContext& ctx, const TString& name);

} // namespace NAlice
