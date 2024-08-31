#include "names.h"

namespace NAlice::NHollywood::NModifiers::NMetrics {

namespace {

constexpr TStringBuf NAME = "name";
constexpr TStringBuf MODIFIER_NAME = "modifier_name";
constexpr TStringBuf MODIFIERS_WPS_SENSOR_NAME = "modifiers_wins_per_second";

} // namespace

TLabels LabelsForModifiersWps(const TString& modifierName) {
    return {{NAME, MODIFIERS_WPS_SENSOR_NAME}, {MODIFIER_NAME, modifierName}};
}

} // namespace NAlice::NHollywood::NModifiers::NMetrics
