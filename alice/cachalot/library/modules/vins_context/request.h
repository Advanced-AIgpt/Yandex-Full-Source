#pragma once

#include <alice/cachalot/library/request.h>
#include <alice/cachalot/library/modules/vins_context/storage.h>


namespace NCachalot {

class TRequestVinsContext : public TRequest {
public:
    using TEmptyResponse = TVinsContextStorage::TEmptyResponse;

    TRequestVinsContext(
        const NNeh::IRequestRef& req,
        TIntrusivePtr<IKvStorage<TVinsContextKey, TString, TVinsExtraData>> storage
    );

    TAsyncStatus ServeAsync() override;

private:
    TIntrusivePtr<TRequestVinsContext> IntrusiveThis() {
        return this;
    }

protected:
    TIntrusivePtr<IKvStorage<TVinsContextKey, TString, TVinsExtraData>> Storage;
};


}   // namespace NCachalot
