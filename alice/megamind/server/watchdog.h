#pragma once

#include <alice/megamind/library/dispatcher/apphost_dispatcher.h>
#include <alice/megamind/library/globalctx/globalctx.h>
#include <alice/megamind/library/registry/registry.h>

namespace NAlice::NMegamind {

class TWatchDogRegistry final : public TRegistry {
public:
    TWatchDogRegistry(TAppHostDispatcher* appHostDispatcher, IGlobalCtx& globalCtx);

    void Add(const TString& path, NAppHost::TAsyncAppService handler) override;
    void Add(const TString& path, NAppHost::TAppService handler) override;
    void Add(const TString& path, NNeh::TServiceFunction handler) override;

private:
    IGlobalCtx& GlobalCtx_;
};

} // namespace NAlice::NMegamind
