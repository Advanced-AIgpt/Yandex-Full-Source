#pragma once

#include <alice/bass/forms/context/context.h>

#include <util/generic/string.h>

namespace NBASS {

enum class EUidAcquireType { UNAUTHORIZED, BLACK_BOX, BIOMETRY };

EUidAcquireType GetUidAcquireType(const TContext& ctx);

TString GetUnauthorizedUid(const TContext& context);

} // namespace NBASS
