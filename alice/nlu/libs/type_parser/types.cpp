#include "types.h"
#include <util/generic/yexception.h>

namespace NAlice {
    TString EntityTypeToString(const EEntityType type) {
        switch (type) {
            case EEntityType::TEXT:
                return "text";
            case EEntityType::DIGITAL:
                return "digital";
            case EEntityType::TIME:
                return "time";
            case EEntityType::UNKNOWN:
                return "unknown";
        }
        Y_ENSURE(false);
    }
} // namespace NAlice
