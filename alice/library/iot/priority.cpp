#include "priority.h"

#include "defs.h"

#include <util/string/builder.h>


namespace NAlice::NIot {

namespace {

/* We prefer matching specific (user) continuous custom entities and penalize
 * matching common (stop) words. */
const THashMap<TString, TPriorityType> ENTITY_TYPE_TO_PRIORITY = {
    {"scenario", 1030},
    {"triggered_scenario", 1030},

    {"device", 1020},
    {"group", 1020},
    {"room", 1020},
    {"household", 1020},

    {"scenario_close_variation", 1015},
    {"triggered_scenario_close_variation", 1015},

    {"device_close_variation", 1010},
    {"group_close_variation", 1010},
    {"room_close_variation", 1010},

    {"device_synonym", 10},
    {"group_synonym", 10},
    {"room_synonym", 10},

    {"device_type", 15},
    {"device_type_close_variation", 10},
    {"device_type_synonym", 5},

    {"instance", -50},
    {"bow_instance", -50},
    {"action_value", -50},
    {"bow_action_value", -50},
    {"common", -150},

    {"fst_DATETIME", -200},
    {"fst_TIME", -250},
    {"fst_DATETIME_RANGE", -300},
};

constexpr TPriorityType CONTINUOUS_PRIORITY_INCREASE = 2500;
constexpr TPriorityType USER_ENTITY_EXACT_MATCHING_INCREASE = 30;
constexpr TPriorityType USER_ENTITY_CLOSE_VARIATION_EXACT_MATCHING_INCREASE = 3;
constexpr TPriorityType CORRECT_PREPOSITION_BEFORE_DATETIME_RANGE_INCREASE = 150;

/* In case of passing to MakeHypotheses several smart homes, we priorize their
 * entities in the order of the corresponding smart homes. */
constexpr TPriorityType NEXT_SMART_HOME_ENTITY_PRIORITY_DECREASE = 10000;

TString CloseVariation(const TStringBuf entityType) {
    return TStringBuilder() << entityType << "_close_variation";
}

TString Synonym(const TStringBuf entityType) {
    return TStringBuilder() << entityType << "_synonym";
}

const THashSet<TString> USER_ENTITY_TYPES = {
    ENTITY_TYPE_SCENARIO, ENTITY_TYPE_TRIGGERED_SCENARIO, ENTITY_TYPE_DEVICE, ENTITY_TYPE_GROUP, ENTITY_TYPE_ROOM,
    CloseVariation(ENTITY_TYPE_SCENARIO), CloseVariation(ENTITY_TYPE_TRIGGERED_SCENARIO), CloseVariation(ENTITY_TYPE_DEVICE),
    CloseVariation(ENTITY_TYPE_GROUP), CloseVariation(ENTITY_TYPE_ROOM)
};

} // namespace

void ChangePriority(const TPriorityType delta, NSc::TValue& hypothesis) {
    hypothesis[FIELD_PRIORITY].SetIntNumber(hypothesis.TrySelect(FIELD_PRIORITY).GetIntNumber(DEFAULT_PRIORITY) + delta);
}

TPriorityType RawEntitiesCoverageToHypothesisPriorityChange(const TParsingHypothesis& ph) {
    const auto& rawEntities = ph.RawEntities();
    TPriorityType result = 0;
    for (const auto& e : rawEntities) {
        TString type = ToString(e.Type());
        if (e.Extra().IsSynonym()) {
            type = Synonym(type);
        } else if (e.Extra().IsCloseVariation()) {
            type = CloseVariation(type);
        }

        auto coverage = e.End() - e.Start();
        auto priority = ENTITY_TYPE_TO_PRIORITY.FindPtr(type);
        if (priority) {
            result += *priority * coverage;
        }

        // Adding bonus for continuous coverage only to entities with non-negative priority
        if (!priority || *priority >= 0) {
            result += (coverage - 1) * CONTINUOUS_PRIORITY_INCREASE;
        }

        if (e.Extra().IsExact() && USER_ENTITY_TYPES.contains(type)) {
            auto increase = e.Extra().IsCloseVariation() ?
                USER_ENTITY_CLOSE_VARIATION_EXACT_MATCHING_INCREASE : USER_ENTITY_EXACT_MATCHING_INCREASE;
            result += increase * coverage;
        }
    }

    return result;
}

TPriorityType CalculatePriorityInMultiSmartHomeEnvironment(const NSc::TValue& hypothesis, const TSmartHomeIndex& shIndex) {
    TPriorityType result = 0;
    for (const auto& entity : hypothesis.Get(FIELD_RAW_ENTITIES).GetArray()) {
        const auto& ids = entity[FIELD_EXTRA][FIELD_IDS];
        if (ids.IsArray()) {
            auto minShNumber = std::numeric_limits<int>::max();
            for (const auto& id : ids.GetArray()) {
                const auto shNumber = shIndex.EntityToSmartHomeNumber.FindPtr(id);
                Y_ENSURE(shNumber);
                minShNumber = Min(minShNumber, *shNumber);
            }
            result -= minShNumber * NEXT_SMART_HOME_ENTITY_PRIORITY_DECREASE;
        }
    }
    return result;
}

void AddBonusForCorrectPrepositionBeforeDatetimeRange(const TParsingHypothesis& ph, NSc::TValue& preliminaryHypothesis) {
    if (IsIn(ph.Actions(0).Flags(), FLAG_DO_NOT_RECOVER_DATETIME_RANGE)) {
        return;
    }

    const auto& rawEntities = ph.RawEntities();
    for (size_t i = 1; i < rawEntities.Size(); ++i) {
        if (rawEntities[i].Type() == "fst_DATETIME_RANGE" && rawEntities[i - 1].Text() == "на") {
            const auto coverage = rawEntities[i].End() - rawEntities[i].Start();
            ChangePriority(CORRECT_PREPOSITION_BEFORE_DATETIME_RANGE_INCREASE * coverage, preliminaryHypothesis);
            return;
        }
    }
}

void AddBonus(const TParsingHypothesis& ph, NSc::TValue& hypothesis) {
    hypothesis[FIELD_PRIORITY].GetIntNumberMutable() += ph.Actions(0).Bonus();
}

} // namespace NAlice::NIot
