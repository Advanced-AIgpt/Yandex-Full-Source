#pragma once

#include "fwd.h"

namespace NAlice {
struct IRng;
} // namespace NAlice

namespace NAlice::NNlg {

INlgRendererPtr CreateNlgRendererFromRegisterFunction(TRegisterFunction registerFunction, IRng& rng);

INlgRendererPtr CreateLocalizedNlgRendererFromRegisterFunction(
    TRegisterFunction registerFunction,
    ITranslationsContainerPtr translationsContainer,
    IRng& rng);

}
