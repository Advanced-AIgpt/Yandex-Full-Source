#include "restriction_level.h"

namespace NAlice {

namespace {

enum EBassOptionFiltrationLevel {
    Without = 0,
    Medium,
    Children,
    Safe,
};

} // namespace

EContentSettings CalculateContentRestrictionLevel(EContentSettings contentSettings, TMaybe<ui32> filtrationLevel) {
    if ((filtrationLevel && *filtrationLevel == EBassOptionFiltrationLevel::Safe) ||
        contentSettings == EContentSettings::safe) {
        return EContentSettings::safe;
    }
    if ((filtrationLevel && *filtrationLevel == EBassOptionFiltrationLevel::Children) ||
        contentSettings == EContentSettings::children) {
        return EContentSettings::children;
    }
    if ((filtrationLevel && *filtrationLevel == EBassOptionFiltrationLevel::Without) ||
        contentSettings == EContentSettings::without) {
        return EContentSettings::without;
    }

    // For now choose moderate search by default, except other filters are selected
    return EContentSettings::medium;
}

EContentRestrictionLevel GetContentRestrictionLevel(EContentSettings level) {
    switch (level) {
        case EContentSettings::children:
            return EContentRestrictionLevel::Children;
        case EContentSettings::without:
            return EContentRestrictionLevel::Without;
        case EContentSettings::safe:
            return EContentRestrictionLevel::Safe;
        case EContentSettings::medium:
            return EContentRestrictionLevel::Medium;
    }
}

EContentSettings ToProtoType(EContentRestrictionLevel level) {
    switch (level) {
        case EContentRestrictionLevel::Medium:
            return EContentSettings::medium;
        case EContentRestrictionLevel::Children:
            return EContentSettings::children;
        case EContentRestrictionLevel::Without:
            return EContentSettings::without;
        case EContentRestrictionLevel::Safe:
            return EContentSettings::safe;
    }
    Y_UNREACHABLE();
}

} // namespace NAlice
