#include <alice/cachalot/library/modules/activation/service.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/activation/request.h>
#include <alice/cachalot/library/modules/activation/storage.h>
#include <alice/cachalot/library/modules/activation/yql_requests.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/neh/neh.h>


namespace NCachalot {

TMaybe<TActivationServiceSettings> TActivationService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.Activation();
}


TActivationService::TActivationService(const TActivationServiceSettings& settings)
    : Storage(MakeActivationStorage(settings.Ydb()))
{
    Storage->SetDefaultTtlSeconds(settings.TtlSeconds());
    Storage->SetFreshnessThresholdMilliSeconds(settings.FreshnessDeltaMilliSeconds());

    // Allocating all possible yql-requests before memory lock
    TActivationStorageYqlCodesStorage::GetInstance().Initialize(settings.Ydb().Database());
}


NThreading::TFuture<void> TActivationService::OnAnnouncementRequest(const NNeh::IRequestRef& request) {
    TMetrics::GetInstance().ActivationMetrics.RequestMetrics.OnStarted();

    TRequestPtr req = MakeIntrusive<TRequestActivationAnnouncement>(request, Storage, Config);
    if (req->IsFinished()) {
        req->ReplyTo(request);
    } else {
        req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
            req->ReplyTo(request);
        });
    }

    return req->GetFinishFuture();
}


NThreading::TFuture<void> TActivationService::OnFinalRequest(const NNeh::IRequestRef& request) {
    TMetrics::GetInstance().ActivationMetrics.RequestMetrics.OnStarted();

    TRequestPtr req = MakeIntrusive<TRequestActivationFinal>(request, Storage, Config);
    if (req->IsFinished()) {
        req->ReplyTo(request);
    } else {
        req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
            req->ReplyTo(request);
        });
    }

    return req->GetFinishFuture();
}


template <typename TRequestActivationClass>
NThreading::TFuture<void> ProcessRequest(
    NAppHost::TServiceContextPtr ctx,
    TIntrusivePtr<IActivationStorage> storage,
    TActivationServiceConfig serviceConfig
) {
    TMetrics::GetInstance().ActivationMetrics.RequestMetrics.OnStarted();

    DLOG("TActivationService::ProcessRequest");

    TRequestPtr req = MakeIntrusive<TRequestActivationClass>(ctx, std::move(storage), std::move(serviceConfig));
    DLOG("TActivationService::ProcessRequest made req object");

    auto replyCallback = [ctx, req](const TAsyncStatus& = TAsyncStatus()) {
        DLOG("TActivationService::ProcessRequest in callback");
        req->ReplyTo(ctx);
    };

    if (req->IsFinished()) {
        DLOG("TActivationService::ProcessRequest instant finish :(");
        replyCallback();
    } else {
        DLOG("TActivationService::ProcessRequest processing");
        req->ServeAsync().Subscribe(replyCallback);
    }

    DLOG("TActivationService::ProcessRequest returning");
    return req->GetFinishFuture();
}


NThreading::TFuture<void> TActivationService::OnVoiceInputFirstAnnouncementRequest(NAppHost::TServiceContextPtr ctx) {
    return ProcessRequest<TRequestVoiceInputActivationFirstAnnouncement>(ctx, Storage, Config);
}

NThreading::TFuture<void> TActivationService::OnVoiceInputSecondAnnouncementRequest(NAppHost::TServiceContextPtr ctx) {
    return ProcessRequest<TRequestVoiceInputActivationSecondAnnouncement>(ctx, Storage, Config);
}

NThreading::TFuture<void> TActivationService::OnVoiceInputFinalRequest(NAppHost::TServiceContextPtr ctx) {
    return ProcessRequest<TRequestVoiceInputActivationFinal>(ctx, Storage, Config);
}


bool TActivationService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, "/activation_announcement", [this](const NNeh::IRequestRef& request) {
        return this->OnAnnouncementRequest(request);
    });
    loop.Add(port, "/activation_final", [this](const NNeh::IRequestRef& request) {
        return this->OnFinalRequest(request);
    });
    loop.Add(port, "/activation_voice_input_first_announcement", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnVoiceInputFirstAnnouncementRequest(ctx);
    });
    loop.Add(port, "/activation_voice_input_second_announcement", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnVoiceInputSecondAnnouncementRequest(ctx);
    });
    loop.Add(port, "/activation_voice_input_final", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnVoiceInputFinalRequest(ctx);
    });
    return true;
}

}   // namespace NCachalot
