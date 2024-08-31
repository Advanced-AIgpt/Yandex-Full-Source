#include "entity_finder.h"

#include <util/generic/vector.h>
#include <util/string/join.h>
#include <util/string/split.h>

#include <algorithm>

namespace NAlice {

namespace NEntitySearch::NEntityFinder {

namespace {

constexpr size_t ENTITY_ID_INDEX = 3;

constexpr TStringBuf ENTITY_FINDER_PATH = "EntityFinder";
constexpr TStringBuf RULES_PATH = "rules";
constexpr TStringBuf WINNER_PATH = "Winner";

} // namespace

TString ExtractEntityId(TStringBuf winner) {
    TVector<TString> parts;

    StringSplitter(winner).Split('\t').ParseInto(&parts);

    if (parts.size() <= ENTITY_ID_INDEX) {
        return {};
    }

    return parts[ENTITY_ID_INDEX];
}

TString GetEntitiesString(const NJson::TJsonValue& vinsWizardResponse) {
    const auto& rules = vinsWizardResponse[RULES_PATH];
    if (!rules.IsMap()) {
        return {};
    }

    const auto& entityFinder = rules[ENTITY_FINDER_PATH];
    if (!entityFinder.IsMap()) {
        return {};
    }

    const auto& winner = entityFinder[WINNER_PATH];

    if (winner.IsString()) {
        return ExtractEntityId(entityFinder[WINNER_PATH].GetString());
    }

    if (winner.IsArray()) {
        TVector<TString> entityIds;

        for (const auto& winnerItem : winner.GetArray()) {
            if (!winnerItem.IsString()) {
                continue;
            }

            const auto entityId = ExtractEntityId(winnerItem.GetString());

            if (!entityId.empty()) {
                entityIds.push_back(entityId);
            }
        }

        return JoinSeq(",", entityIds);
    }

    return {};
}

} // namespace NEntitySearch::NEntityFinder

} // namespace NAlice
