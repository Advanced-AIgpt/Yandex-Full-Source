#pragma once

#include <alice/cachalot/library/request.h>
#include <alice/cachalot/library/modules/yabio_context/storage.h>


namespace NCachalot {


class TRequestYabioContext : public TRequest {
public:
    TRequestYabioContext(
        const NNeh::IRequestRef& req,
        TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> storage
    );

    TRequestYabioContext(
        NAppHost::TServiceContextPtr ctx,
        TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> storage
    );

    TAsyncStatus ServeAsync() override;

private:
    TIntrusivePtr<TRequestYabioContext> IntrusiveThis() {
        return this;
    }

    TAsyncStatus ServeAsyncSave();
    TAsyncStatus ServeAsyncLoad();
    TAsyncStatus ServeAsyncDelete();

    void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx) override;
    void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx) override;

protected:
    TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> Storage;
};


}   // namespace NCachalot
