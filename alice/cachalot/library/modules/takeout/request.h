#pragma once

#include <alice/cachalot/library/request.h>
#include <alice/cachalot/library/modules/takeout/storage.h>


namespace NCachalot {

class TRequestTakeout : public TRequest {
public:
    TRequestTakeout(const NNeh::IRequestRef& req, TIntrusivePtr<ITakeoutResultsStorage> storage);

    TAsyncStatus ServeAsync() override;

private:
    TIntrusivePtr<TRequestTakeout> IntrusiveThis() {
        return this;
    }

protected:
    TIntrusivePtr<ITakeoutResultsStorage> Storage;
};


}   // namespace NCachalot
