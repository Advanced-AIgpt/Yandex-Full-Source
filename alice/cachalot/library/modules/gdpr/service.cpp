#include <alice/cachalot/library/modules/gdpr/service.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/gdpr/request.h>
#include <alice/cachalot/library/modules/gdpr/storage.h>

#include <alice/cuttlefish/library/logging/dlog.h>

using namespace NCachalot;
using namespace NCachalotProtocol;


TMaybe<TGDPRServiceSettings> TGDPRService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.GDPR();
}

TGDPRService::TGDPRService(const NCachalot::TGDPRServiceSettings& settings)
    : Storage(MakeGDPRStorage(settings)) {
}

void TGDPRService::OnGDPRRequest(const NNeh::IRequestRef& request) {
    TMetrics::GetInstance().GDPRMetrics.RequestMetrics.OnStarted();
    DLOG("TGDPRService::OnRequest Started");
    TRequestPtr req = MakeIntrusive<TRequestGDPR>(request, Storage);
    if (req->IsFinished()) {
        req->ReplyTo(request);
        return;
    }

    DLOG(TInstant::Now().ToString() << " TGDPRService::OnLocationRequest ServeAsync");
    req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
        req->ReplyTo(request);
    });
}

bool TGDPRService::Integrate(NAppHost::TLoop& loop, ui16 port) {
    loop.Add(port, "/gdpr", [this](const NNeh::IRequestRef& request) {
        this->OnGDPRRequest(request);
    });

    return true;
}
