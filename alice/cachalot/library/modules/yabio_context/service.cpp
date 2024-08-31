#include <alice/cachalot/library/modules/yabio_context/service.h>

#include <alice/cachalot/library/debug.h>
#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/mock.h>

#include <alice/cuttlefish/library/logging/dlog.h>

using namespace NCachalot;
using namespace NCachalotProtocol;
using namespace NAlice::NCuttlefish;


TMaybe<TYabioContextServiceSettings> TYabioContextService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.YabioContext();
}

TYabioContextService::TYabioContextService(const TYabioContextServiceSettings& settings)
    : Storage(MakeYabioContextYdbStorage(settings.Storage()))
{
}

template <typename TRequestSource>
NThreading::TFuture<void> TYabioContextService::OnRequest(const TRequestSource& requestSource) {
    TMetrics::GetInstance().YabioContextServiceMetrics.Request.OnStarted();
    DLOG("TYabioContextService::OnRequest<" << typeid(requestSource).name() << "> Started");

    TRequestPtr req = MakeIntrusive<TRequestYabioContext>(requestSource, Storage);

    if (IsSuspended()) {
        req->SetError(EResponseStatus::SERVICE_UNAVAILABLE, "Service is suspended");
    }

    if (req->IsFinished()) {
        req->ReplyTo(requestSource);
    } else {
        req->ServeAsync().Subscribe([requestSource, req](const TAsyncStatus&) {
            req->ReplyTo(requestSource);
        });
    }

    return req->GetFinishFuture();
}


bool TYabioContextService::Integrate(NAppHost::TLoop& loop, ui16 port) {
    loop.Add(port, "/yabio_context", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnRequest(ctx);
    });
    loop.Add(port, "/yabio_context", [this](const NNeh::IRequestRef& request) {
        return this->OnRequest(request);
    });

    // TODO (yakovdom): remove following handler
    loop.Add(port, "/yabio_context/v2", [this](const NNeh::IRequestRef& request) {
        return this->OnRequest(request);
    });

    return true;
}
