#include "table.h"

#include <alice/megamind/library/classifiers/util/scenario_info.h>

#include <util/generic/is_in.h>

namespace NAlice {

TClassificationTable::TClassificationTable(const TVector<TStringBuf> scenarios)
    : Scenarios(scenarios)
{}

void TClassificationTable::SetDirectMarker(const TStringBuf scenario, const EClassificationMarker marker) {
    SetMarker(scenario, marker, DirectMarkers);
}
void TClassificationTable::SetStrongMarker(const TStringBuf scenario, const EClassificationMarker marker) {
    SetMarker(scenario, marker, StrongMarkers);
}
void TClassificationTable::SetNecessaryMarker(const TStringBuf scenario, const EClassificationMarker marker) {
    SetMarker(scenario, marker, NecessaryMarkers);
}

void TClassificationTable::SetEmptyDirectMarker(const EClassificationMarker marker) {
    SetEmptyMarker(marker, DirectMarkers);
}
void TClassificationTable::SetEmptyStrongMarker(const EClassificationMarker marker) {
    SetEmptyMarker(marker, StrongMarkers);
}
void TClassificationTable::SetEmptyNecessaryMarker(const EClassificationMarker marker) {
    SetEmptyMarker(marker, NecessaryMarkers);
}

void TClassificationTable::SetDirectMarkersLossReason(const ELossReason lossReason) {
    DirectMarkersLossReason = lossReason;
}
void TClassificationTable::SetStrongMarkersLossReason(const ELossReason lossReason) {
    StrongMarkersLossReason = lossReason;
}
void TClassificationTable::SetNecessaryMarkersLossReason(const TStringBuf scenario, const ELossReason lossReason) {
    if (!NecessaryMarkersLossReasons.contains(scenario)) {
        NecessaryMarkersLossReasons[scenario] = lossReason;
    }
}

bool TClassificationTable::AllNecessary(const TStringBuf scenario) const {
    bool allNecessary = true;
    for (const auto& [markerName, markerScenarios] : NecessaryMarkers) {
        allNecessary = allNecessary && markerScenarios.at(scenario);
    }
    return allNecessary;
}

TVector<TStringBuf> TClassificationTable::GetAllNecessary() const {
    TVector<TStringBuf> allNecessaryScenarios;
    for (const auto scenario : Scenarios) {
        if (AllNecessary(scenario)) {
            allNecessaryScenarios.push_back(scenario);
        }
    }
    return allNecessaryScenarios;
}

TVector<TStringBuf> TClassificationTable::GetWinners(TQualityStorage& qualityStorage) const {
    TVector<TStringBuf> winners;
    if (!DirectMarkers.empty()) {
        for (const auto& scenario : Scenarios) {
            bool winner = true;
            for (const auto& [markerName, markerScenarios] : DirectMarkers) {
                winner = winner && markerScenarios.at(scenario);
            }
            if (winner) {
                winners.push_back(scenario);
            } else {
                UpdateScenarioClassificationInfo(qualityStorage, DirectMarkersLossReason, TString(scenario), ECS_PRE);
            }
        }
        return winners;
    }
    for (const auto& scenario : Scenarios) {
        if (AllNecessary(scenario)) {
            if (StrongMarkers.empty()) {
                winners.push_back(scenario);
            } else {
                bool anyStrong = false;
                for (const auto& [markerName, markerScenarios] : StrongMarkers) {
                    anyStrong = anyStrong || markerScenarios.at(scenario);
                }
                if (anyStrong) {
                    winners.push_back(scenario);
                } else {
                    UpdateScenarioClassificationInfo(qualityStorage, StrongMarkersLossReason, TString(scenario), ECS_PRE);
                }
            }
        } else {
            UpdateScenarioClassificationInfo(qualityStorage, NecessaryMarkersLossReasons.at(scenario), TString(scenario), ECS_PRE);
        }
    }
    return winners;

}

bool TClassificationTable::StrongMarkersExist() const {
    return !StrongMarkers.empty();
}

void TClassificationTable::SetMarker(const TStringBuf scenario, const EClassificationMarker marker, TMarkersTable& markersTable) {
    if (!IsIn(markersTable, marker)) {
        SetEmptyMarker(marker, markersTable);
    }
    if (IsIn(Scenarios, scenario)) {
        markersTable.at(marker).at(scenario) = true;
    }
}

void TClassificationTable::SetEmptyMarker(const EClassificationMarker marker, TMarkersTable& markersTable) {
    if (markersTable.find(marker) != markersTable.end()) {
        return;
    }
    THashMap<TStringBuf, bool> markerScenarios;
    for (const auto& scenarioInternal : Scenarios) {
        markerScenarios.emplace(scenarioInternal, false);
    }
    markersTable.emplace(marker, markerScenarios);
}

} // namespace NAlice
