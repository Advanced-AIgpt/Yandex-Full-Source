#include "fake_registry.h"

#include <alice/megamind/library/util/wildcards.h>

namespace NAlice::NMegamind {

TFakeRegistry::TFakeRegistry()
    : TRegistry{nullptr}
{
}

void TFakeRegistry::Add(const TString& path, NAppHost::TAsyncAppService /* handler */) {
    AppHostHandlers.emplace(path);
}

void TFakeRegistry::Add(const TString& path, NAppHost::TAppService /* handler */) {
    AppHostHandlers.emplace(path);
}

void TFakeRegistry::Add(const TString& path, NNeh::TServiceFunction /* handler */) {
    AppHostHandlers.emplace(path);
}


} // namespace NAlice::NMegamind
