#pragma once

#include "fwd.h"
#include <util/generic/fwd.h>

namespace NAlice {
struct IRng;
} // namespace NAlice

namespace NAlice::NNlg {

INlgRendererPtr CreateNlgRendererFromNlgLibraryPath(const TStringBuf nlgLibraryPath, IRng& rng);

INlgRendererPtr CreateLocalizedNlgRendererFromNlgLibraryPath(
    const TStringBuf nlgLibraryPath,
    ITranslationsContainerPtr translationsContainer,
    IRng& rng);

}
