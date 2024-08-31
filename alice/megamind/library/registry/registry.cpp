#include "registry.h"

#include <alice/megamind/library/dispatcher/apphost_dispatcher.h>

namespace NAlice {

TRegistry::TRegistry(NMegamind::TAppHostDispatcher* appHostDispatcher)
    : AppHostDispatcher{appHostDispatcher}
{
}

void TRegistry::Add(const TString& path, NAppHost::TAsyncAppService handler) {
    if (AppHostDispatcher) {
        AppHostDispatcher->Add(path, handler);
    }
}

void TRegistry::Add(const TString& path, NAppHost::TAppService handler) {
    if (AppHostDispatcher) {
        AppHostDispatcher->Add(path, handler);
    }
}

void TRegistry::Add(const TString& path, NNeh::TServiceFunction handler) {
    if (AppHostDispatcher) {
        AppHostDispatcher->Add(path, handler);
    }
}

} // namespace NAlice
