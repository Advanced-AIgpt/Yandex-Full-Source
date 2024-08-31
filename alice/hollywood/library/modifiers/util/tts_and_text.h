#pragma once

#include <util/generic/string.h>

namespace NAlice::NHollywood::NModifiers {

struct TTtsAndText final {
    TTtsAndText(const TString& tts, const TString& Text);

    TString Tts;
    TString Text;
};

} // namespace NAlice::NHollywood::NModifiers
