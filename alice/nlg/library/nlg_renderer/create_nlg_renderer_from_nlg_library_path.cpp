#include "create_nlg_renderer_from_nlg_library_path.h"
#include "create_nlg_renderer_from_register_function.h"
#include "nlg_renderer.h"
#include <alice/nlg/library/runtime_api/nlg_library_registry.h>

namespace NAlice::NNlg {

INlgRendererPtr CreateNlgRendererFromNlgLibraryPath(const TStringBuf nlgLibraryPath, IRng& rng) {
    return CreateLocalizedNlgRendererFromNlgLibraryPath(nlgLibraryPath, nullptr, rng);
}

INlgRendererPtr CreateLocalizedNlgRendererFromNlgLibraryPath(
    const TStringBuf nlgLibraryPath,
    ITranslationsContainerPtr translationsContainer,
    IRng& rng)
{
    auto registerFunction = TNlgLibraryRegistry::Instance().TryGetRegisterFunctionByNlgLibraryPath(nlgLibraryPath);
    Y_ENSURE(registerFunction, "Cannot find nlg register function for nlg library path " << nlgLibraryPath);
    return CreateLocalizedNlgRendererFromRegisterFunction(
        std::move(registerFunction), std::move(translationsContainer), rng);
}

}
