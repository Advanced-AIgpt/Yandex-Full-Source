#pragma once

#include <alice/library/logger/logger.h>
#include <alice/megamind/library/dispatcher/fwd.h>

#include <apphost/api/service/cpp/service_loop.h>

#include <util/generic/noncopyable.h>
#include <util/generic/string.h>

namespace NAlice {

class TRegistry : private NNonCopyable::TNonCopyable {
public:
    TRegistry(NMegamind::TAppHostDispatcher* appHostDispatcher);
    virtual ~TRegistry() = default;

    virtual void Add(const TString& path, NAppHost::TAsyncAppService handler);
    virtual void Add(const TString& path, NAppHost::TAppService handler);
    virtual void Add(const TString& path, NNeh::TServiceFunction handler);

private:
    NMegamind::TAppHostDispatcher* AppHostDispatcher;
};

} // namespace NAlice
