#include "names.h"

namespace NAlice::NHollywood::NCombinators::NMetrics {

namespace {

constexpr TStringBuf NAME = "name";
constexpr TStringBuf COMBINATOR_NAME = "combinator_name";
constexpr TStringBuf SCENARIO_NAME = "scenario_name";
constexpr TStringBuf COMBINATOR_MISSED_SCENARIOS_SENSOR_NAME = "combinator_missed_scenarios_per_second";

} // namespace

TLabels LabelsCombinatorMissedScenarios(const TString& combinatorName, const TString& scenarioName) {
    return {{NAME, COMBINATOR_MISSED_SCENARIOS_SENSOR_NAME}, {COMBINATOR_NAME, combinatorName}, {SCENARIO_NAME, scenarioName}};
}

} // namespace NAlice::NHollywood::NCombinators::NMetrics
