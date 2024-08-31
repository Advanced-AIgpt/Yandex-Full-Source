#pragma once

#include <alice/cachalot/library/request.h>
#include <alice/cachalot/library/modules/gdpr/storage.h>


namespace NCachalot {

class TRequestGDPR : public TRequest {
public:
    TRequestGDPR(const NNeh::IRequestRef& req, TIntrusivePtr<IGDPRStorage> storage);

    TAsyncStatus ServeAsync() override;

private:
    TIntrusivePtr<TRequestGDPR> IntrusiveThis() {
        return this;
    }

protected:
    TIntrusivePtr<IGDPRStorage> Storage;
};


}   // namespace NCachalot
