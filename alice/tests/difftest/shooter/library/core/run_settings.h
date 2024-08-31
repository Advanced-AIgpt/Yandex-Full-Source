#pragma once

#include <alice/tests/difftest/shooter/library/core/config.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>

namespace NAlice::NShooter {

using TRunSettingsImpl = NShooterConfig::TConfigConst<TSchemeTraits>::TRunSettingsConst;

struct TRunSettings : public NNonCopyable::TMoveOnly {
    TRunSettings() = default;
    TRunSettings(const TRunSettingsImpl& runSettingsImpl);

    TString PackagePath;
    TString LogsPath;
    TString ResponsesPath;
    TString Config;

    TVector<TString> EnabledExperiments;
    TVector<TString> DisabledExperiments;

    bool EnableIdleMode;
    bool EnablePerfMode;
    bool EnableHollywoodMode;
    bool EnableHollywoodBassMode;
    bool IncreaseTimeouts;
    bool DontClose;
};

} // namespace NAlice::NShooter
