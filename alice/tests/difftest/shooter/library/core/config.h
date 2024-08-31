#pragma once

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <alice/tests/difftest/shooter/library/core/config.sc.h>

namespace NAlice::NShooter {

class TConfig : private NSc::TValue, public NShooterConfig::TConfig<TSchemeTraits>, NNonCopyable::TNonCopyable {
public:
    TConfig(const TString& configFileName);

    using NSc::TValue::ToJson;

private:
    using TScheme = NShooterConfig::TConfig<TSchemeTraits>;
};

} // namespace NAlice::NShooter
