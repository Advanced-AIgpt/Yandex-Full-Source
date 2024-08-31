#include "run_settings.h"

namespace NAlice::NShooter {

namespace {

TVector<TString> ConstructEnabledExperiments(const TRunSettingsImpl& runSettingsImpl) {
    if (!runSettingsImpl.HasEnabledExperiments()) {
        return {};
    }
    TVector<TString> exps;
    for (const auto& exp : runSettingsImpl.EnabledExperiments()) {
        exps.emplace_back(exp);
    }
    return exps;
}

TVector<TString> ConstructDisabledExperiments(const TRunSettingsImpl& runSettingsImpl) {
    if (!runSettingsImpl.HasDisabledExperiments()) {
        return {};
    }
    TVector<TString> exps;
    for (const auto& exp : runSettingsImpl.DisabledExperiments()) {
        exps.emplace_back(exp);
    }
    return exps;
}

} // namespace

TRunSettings::TRunSettings(const TRunSettingsImpl& runSettingsImpl)
    : PackagePath{runSettingsImpl.PackagePath()}
    , LogsPath{runSettingsImpl.LogsPath()}
    , ResponsesPath{runSettingsImpl.ResponsesPath()}
    , Config{runSettingsImpl.Config()}
    , EnabledExperiments{ConstructEnabledExperiments(runSettingsImpl)}
    , DisabledExperiments{ConstructDisabledExperiments(runSettingsImpl)}
    , EnableIdleMode{runSettingsImpl.EnableIdleMode()}
    , EnablePerfMode{runSettingsImpl.EnablePerfMode()}
    , EnableHollywoodMode{runSettingsImpl.EnableHollywoodMode()}
    , EnableHollywoodBassMode{runSettingsImpl.EnableHollywoodBassMode()}
    , IncreaseTimeouts{runSettingsImpl.IncreaseTimeouts()}
    , DontClose{runSettingsImpl.DontClose()}
{
}

} // namespace NAlice::NShooter
