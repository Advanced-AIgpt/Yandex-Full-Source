#include <alice/cachalot/library/modules/cache/service.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/modules/cache/key_converter.h>
#include <alice/cachalot/library/modules/cache/request.h>
#include <alice/cachalot/library/modules/cache/request_factory.h>

#include <alice/cuttlefish/library/logging/dlog.h>

#include <library/cpp/neh/neh.h>


namespace NCachalot {


TMaybe<TCacheServiceSettings> TCacheService::FindServiceSettings(const TApplicationSettings& settings) {
    return settings.Cache();
}

TCacheService::TCacheService(const TCacheServiceSettings& settings)
    : StorageTag2Context(settings)
{
}

NThreading::TFuture<void> TCacheService::OnHttpRequest(const NNeh::IRequestRef& request) {
    TMetrics::GetInstance().CacheServiceMetrics.Request.OnStarted();

    TRequestPtr req = MakeIntrusive<TRequestCache>(request, StorageTag2Context.Default()); // TODO: support StorageTag in HTTP request as we do for ApphostRequest

    if (IsSuspended()) {
        req->SetError(EResponseStatus::SERVICE_UNAVAILABLE, "Service is suspended");
    }

    if (req->IsFinished()) {
        req->ReplyTo(request);
    } else {
        req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
            req->ReplyTo(request);
        });
    }

    return req->GetFinishFuture();
}

NThreading::TFuture<void> TCacheService::ServeRequest(
    TRequestPtr req,
    NAppHost::TServiceContextPtr ctx,
    TString requestItemTag
) {
    TMetrics::GetInstance().CacheServiceMetrics.Request.OnStarted();
    DLOG("TCacheService::OnApphostRequestImpl start");

    auto replyCallback = [req, ctx, requestItemTag = std::move(requestItemTag)](const TAsyncStatus& = TAsyncStatus()) mutable {
        req->ReplyTo(ctx, std::move(requestItemTag));
    };

    if (req->IsFinished()) {
        DLOG("TCacheService::OnApphostRequestImpl instant finish :(");
        replyCallback();
    } else {
        DLOG("TCacheService::OnApphostRequestImpl processing");
        req->ServeAsync().Subscribe(replyCallback);
    }

    DLOG("TCacheService::OnApphostRequestImpl returning");
    return req->GetFinishFuture();
}

template<typename TProtoRequest, typename TRequestFactory>
NThreading::TFuture<void> TCacheService::UnwrapApphostRequest(
    NAppHost::TServiceContextPtr ctx,
    TRequestFactory& requestFactory
) {
    const auto& refs = ctx->GetProtobufItemRefs(NAppHost::EContextItemSelection::Input);

    return ForEachProtobufItemAsync<TProtoRequest>(
        refs.begin(), refs.end(),
        [this, ctx, &requestFactory](TString requestItemTag, TProtoRequest protoItem) mutable noexcept {
            try {
                DLOG(TStringBuilder() << "TCacheService::UnwrapApphostRequest: type: '" << requestItemTag << "', key: '" << protoItem.GetKey() << "'");

                auto cacheRequest = requestFactory(std::move(protoItem), StorageTag2Context, ctx);
                return ServeRequest(cacheRequest, ctx, std::move(requestItemTag));
            } catch (...) {
                // TODO some metrics
                DLOG("TCacheService::UnwrapApphostRequest unable to parse input item: " + CurrentExceptionMessage());
                return NThreading::MakeFuture();
            }
        });
}

NThreading::TFuture<void> TCacheService::OnApphostGetRequest(NAppHost::TServiceContextPtr ctx) {
    DLOG("TCacheService::OnApphostGetRequest");
    return UnwrapApphostRequest<NCachalotProtocol::TGetRequest>(ctx, MakeCacheGetRequestFromProto);
}

NThreading::TFuture<void> TCacheService::OnApphostSetRequest(NAppHost::TServiceContextPtr ctx) {
    DLOG("TCacheService::OnApphostSetRequest");
    return UnwrapApphostRequest<NCachalotProtocol::TSetRequest>(ctx, MakeCacheSetRequestFromProto);
}

NThreading::TFuture<void> TCacheService::OnApphostDeleteRequest(NAppHost::TServiceContextPtr ctx) {
    DLOG("TCacheService::OnApphostDeleteRequest");
    return UnwrapApphostRequest<NCachalotProtocol::TDeleteRequest>(ctx, MakeCacheDeleteRequestFromProto);
}

bool TCacheService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, "/cache", [this](const NNeh::IRequestRef& request) {
        return this->OnHttpRequest(request);
    });
    loop.Add(port, "/cache_get", [this](const NNeh::IRequestRef& request) {
        return this->OnHttpRequest(request);
    });
    loop.Add(port, "/cache_set", [this](const NNeh::IRequestRef& request) {
        return this->OnHttpRequest(request);
    });
    loop.Add(port, "/cache_delete", [this](const NNeh::IRequestRef& request) {
        return this->OnHttpRequest(request);
    });
    loop.Add(port, "/cache_get_grpc", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnApphostGetRequest(ctx);
    });
    loop.Add(port, "/cache_set_grpc", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnApphostSetRequest(ctx);
    });
    loop.Add(port, "/cache_delete_grpc", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnApphostDeleteRequest(ctx);
    });
    return true;
}


}   // namespace NCachalot
