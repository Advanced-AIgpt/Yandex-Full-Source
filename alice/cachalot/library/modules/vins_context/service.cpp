#include <alice/cachalot/library/modules/vins_context/service.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/mock.h>

#include <alice/cuttlefish/library/logging/dlog.h>

using namespace NCachalot;
using namespace NCachalotProtocol;


TMaybe<TVinsContextServiceSettings> TVinsContextService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.VinsContext();
}

TVinsContextService::TVinsContextService(const TVinsContextServiceSettings& settings)
    : Storage(MakeStorage<TVinsContextStorage>(settings.Ydb().IsFake(), settings.Ydb()))
{
}

void TVinsContextService::OnHttpRequest(const NNeh::IRequestRef& request) {
    TMetrics::GetInstance().VinsContextMetrics()->OnStarted();
    DLOG("TVinsContextService::OnHttpRequest Started");
    TRequestPtr req = MakeIntrusive<TRequestVinsContext>(request, Storage);
    if (req->IsFinished()) {
        req->ReplyTo(request);
        return;
    }

    req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
        req->ReplyTo(request);
    });
}


bool TVinsContextService::Integrate(NAppHost::TLoop& loop, ui16 port) {
    loop.Add(port, "/vins_context/v1", [this](const NNeh::IRequestRef& request) {
        this->OnHttpRequest(request);
    });

    return true;
}
