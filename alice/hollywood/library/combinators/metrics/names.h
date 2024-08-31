#pragma once

#include <library/cpp/monlib/metrics/labels.h>

namespace NAlice::NHollywood::NCombinators::NMetrics {

using namespace NMonitoring;

TLabels LabelsCombinatorMissedScenarios(const TString& combinatorName, const TString& scenarioName);

} // namespace NAlice::NHollywood::NCombinators::NMetrics
