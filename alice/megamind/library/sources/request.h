#pragma once

#include <alice/megamind/library/util/status.h>

namespace NAlice {

enum class ESourcePrepareType {
    Succeeded,
    NotNeeded
};
using TSourcePrepareStatus = TErrorOr<ESourcePrepareType>;

} // namespace NAlice
