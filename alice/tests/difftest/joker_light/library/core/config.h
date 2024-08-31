#pragma once

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/domscheme_traits.h>

#include <alice/tests/difftest/joker_light/library/core/config.sc.h>

namespace NAlice::NJokerLight {

class TConfig : private NSc::TValue, public NJokerLightConfig::TConfig<TSchemeTraits>, NNonCopyable::TNonCopyable {
public:
    TConfig(const TString& configFileName);

    using NSc::TValue::ToJson;

private:
    using TScheme = NJokerLightConfig::TConfig<TSchemeTraits>;
};

} // namespace NAlice::NJokerLight
