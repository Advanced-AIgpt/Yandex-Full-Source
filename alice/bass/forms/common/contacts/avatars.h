#pragma once

#include <alice/bass/forms/context/context.h>

#include <library/cpp/scheme/scheme.h>

namespace NBASS {
namespace NContacts {
namespace NAvatars {

TString MakeUrl(const NSc::TValue& contact, const TContext& ctx);

} // namespace NAvatars
} // namespace NContacts
} // namespace NBASS
