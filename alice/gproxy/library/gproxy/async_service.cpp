#include "async_service.h"

#include <alice/gproxy/library/gproxy/async_call.h>
#include <alice/gproxy/library/protos/service.gproxy.pb.h>


namespace NGProxy {

namespace {

template <typename TFirstMethod, typename TRestMethods>
struct TMethodIteratorImpl {
    static void InitMethod(TAsyncService& service) {
        TMethodIteratorImpl<TFirstMethod, TNone>::InitMethod(service);
        TMethodIteratorImpl<typename TRestMethods::THead, typename TRestMethods::TTail>::InitMethod(service);
    }
};

template <typename TMethod>
struct TMethodIteratorImpl<TMethod, TNone> {
    static void InitMethod(TAsyncService& service) {
        new TAsyncCall<TMethod>(service);
    }
};

}

template <typename TMethodList>
class TMethodIterator {
public:
    static void InitMethodRpcs(TAsyncService& service) {
        TMethodIteratorImpl<typename TMethodList::THead, typename TMethodList::TTail>::InitMethod(service);
    }
};


using TTraits = typename ::NGProxyTraits::TGProxyService<::NGProxy::GProxy>;
using TMethodList = typename TTraits::TMethodList;



void TAsyncService::DoRpcs() {
    if (!GetQueue()) return;
    if (!Get()) return;

    TMethodIterator<TMethodList>::InitMethodRpcs(*this);

    IAsyncCall *tag = nullptr;
    bool ok = true;
    while (ok) {
        CompletionQueue_->Next(reinterpret_cast<void**>(&tag), &ok);
        if (!ok) {
            delete tag;
            ok = true;
            continue;
        }
        if (tag) {
            tag->Do();
        }
    }
}


}   // namespace NGProxy
