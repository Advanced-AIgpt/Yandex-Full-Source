#include <alice/cachalot/library/modules/takeout/service.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/takeout/request.h>
#include <alice/cachalot/library/modules/takeout/storage.h>

#include <alice/cuttlefish/library/logging/dlog.h>

using namespace NCachalot;
using namespace NCachalotProtocol;


TMaybe<TTakeoutServiceSettings> TTakeoutService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.Takeout();
}

TTakeoutService::TTakeoutService(const TTakeoutServiceSettings& settings)
    : Storage(MakeTakeoutResultsStorage(settings.Ydb()))
{
}

void TTakeoutService::OnTakeoutRequest(const NNeh::IRequestRef& request) {
    TMetrics::GetInstance().TakeoutMetrics.RequestMetrics.OnStarted();
    DLOG("TTakeoutService::OnRequest Started");
    TRequestPtr req = MakeIntrusive<TRequestTakeout>(request, Storage);
    if (req->IsFinished()) {
        req->ReplyTo(request);
        return;
    }

    req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
        req->ReplyTo(request);
    });
}

bool TTakeoutService::Integrate(NAppHost::TLoop& loop, ui16 port) {
    loop.Add(port, "/takeout", [this](const NNeh::IRequestRef& request) {
        this->OnTakeoutRequest(request);
    });

    return true;
}
