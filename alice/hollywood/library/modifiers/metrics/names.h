#pragma once

#include <library/cpp/monlib/metrics/labels.h>

namespace NAlice::NHollywood::NModifiers::NMetrics {

using namespace NMonitoring;

TLabels LabelsForModifiersWps(const TString& modifierName);

} // namespace NAlice::NHollywood::NModifiers::NMetrics
