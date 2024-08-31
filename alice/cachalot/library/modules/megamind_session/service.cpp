#include <alice/cachalot/library/modules/megamind_session/service.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/mock.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>

#include <voicetech/library/itags/itags.h>

#include <library/cpp/digest/md5/md5.h>
#include <library/cpp/neh/neh.h>


namespace NCachalot {

namespace {

constexpr TStringBuf ITEM_TYPE_REQUEST = "request";
constexpr TStringBuf ITEM_TYPE_RESPONSE = "response";

ui64 ComputeShardKey(const TString& key) {
    // same as https://a.yandex-team.ru/arc/trunk/arcadia/alice/uniproxy/library/vins_context_storage/__init__.py?rev=6884881#L132
    static_assert(sizeof(ui64) == sizeof(unsigned long long int));
    TString md5LastHalf = MD5::Calc(key).substr(16);
    return std::strtoull(md5LastHalf.data(), nullptr, 16);
}

template<typename TMessage>
TMegamindSessionKey ConstructKeyFromRequestImpl(const TMessage& request) {
    TMegamindSessionKey key;

    key.ShardKey = ComputeShardKey(request.GetUuid());

    TStringBuilder keySb;
    keySb << request.GetUuid();
    if (request.HasDialogId() && request.GetDialogId()) {
        keySb << "$" << request.GetDialogId();
    }
    if (request.HasRequestId() && request.GetRequestId()) {
        keySb << "@" << request.GetRequestId();
    }
    key.Key = std::move(keySb);

    return key;
}

void SetException(const TString& msg, NThreading::TPromise<NCachalotProtocol::TResponse>& promise) {
    DLOG("TMegamindSessionService::OnRequest: " << msg);
    NCachalotProtocol::TResponse response;
    response.SetStatus(EResponseStatus::INTERNAL_ERROR);
    response.SetStatusMessage(TStringBuilder{} << "Got exception: " << msg.Quote());
    promise.SetValue(std::move(response));
}

} // namespace

TMegamindSessionKey ConstructKeyFromRequest(const NCachalotProtocol::TMegamindSessionRequest& request) {
    Y_ASSERT(request.HasLoadRequest() || request.HasStoreRequest());
    if (request.HasLoadRequest()) {
        return ConstructKeyFromRequestImpl(request.GetLoadRequest());
    } else {
        return ConstructKeyFromRequestImpl(request.GetStoreRequest());
    }
}


TMaybe<TMegamindSessionServiceSettings> TMegamindSessionService::FindServiceSettings(
    const TApplicationSettings& settings
) {
    return settings.MegamindSession();
}

TMegamindSessionService::TMegamindSessionService(const TMegamindSessionServiceSettings& settings)
    : Storage(MakeIntrusive<TMegamindSessionStorage>(
        MakeInmemoryStorage<TMegamindSessionKey, TMegamindSessionData>(
            settings.Storage().Imdb(),
            &TMetrics::GetInstance().MegamindSessionMetrics.Imdb
        ),
        MakeStorage<TMegamindSessionYdbStorage>(
            settings.Storage().Ydb().IsFake(),
            settings.Storage().Ydb()
        )
    ))
{
}

NThreading::TFuture<NCachalotProtocol::TResponse> TMegamindSessionService::OnRequest(
    NCachalotProtocol::TMegamindSessionRequest req, const TInstant startTime,
    TMegamindSessionSubMetrics* metrics, TChroniclerPtr logFrame
) {

    auto promise = NThreading::NewPromise<NCachalotProtocol::TResponse>();
    auto result = promise.GetFuture();

    switch (req.GetRequestCase()) {
        case NCachalotProtocol::TMegamindSessionRequest::kLoadRequest: {
            auto& loadMetrics = metrics->Load;
            loadMetrics.OnStart(startTime);

            try {
                const auto key = ConstructKeyFromRequest(req);
                logFrame->LogEvent(NEvClass::MMLoadStarted());
                logFrame->LogEvent(NEvClass::RequestKey(key.Key));

                auto storage = SelectStorageForRequest(req.GetLoadRequest());

                SubscribeWithTryCatchWrapper<NEvClass::MMLoadInternalError>(
                    storage->GetSingleRow(key, logFrame),
                    [promise = promise, loadMetrics, startTime, logFrame, key](
                        const NThreading::TFuture<TMegamindSessionStorage::TSingleRowResponse>& fut
                    ) mutable {
                        const auto& row = fut.GetValueSync();

                        NCachalotProtocol::TResponse response;
                        response.SetStatus(row.Status);

                        if (row.Status == EResponseStatus::OK) {
                            Y_ASSERT(row.Data.Defined());

                            loadMetrics.OnYdbOk(startTime);

                            const TString session = row.Data.GetRef().Data;
                            loadMetrics.SetSize(session.size());
                            // loadMetrics.SetCompressedSize(NBlockCodecs::Codec("lz4")->Encode(session).size());

                            response.MutableMegamindSessionLoadResp()->SetData(session);

                            DLOG("TMegamindSessionService::OnRequest finished successfully");
                            logFrame->LogEvent(NEvClass::MMLoadSuccess());
                        } else {
                            if (row.Status == EResponseStatus::NO_CONTENT) {
                                loadMetrics.OnNotFound(startTime);
                                logFrame->LogEvent(NEvClass::MMLoadNotFound());
                            } else {
                                loadMetrics.OnYdbError(startTime);
                                logFrame->LogEvent(NEvClass::MMLoadFail(row.Stats.ErrorMessage, row.Status));
                            }

                            response.SetStatusMessage(row.Stats.ErrorMessage);
                            DLOG("TMegamindSessionService::OnRequest error (code " <<
                                static_cast<int>(row.Status) << ") : " << row.Stats.ErrorMessage.Quote());
                        }

                        promise.SetValue(std::move(response));

                        loadMetrics.OnOk(startTime);
                    },
                    logFrame
                );
            } catch (...) {
                const TString& msg = CurrentExceptionMessage();
                loadMetrics.OnInternalError(startTime);
                logFrame->LogEvent(NEvClass::MMLoadInternalError(msg));
                SetException(msg, promise);
            }
            break;
        }
        case NCachalotProtocol::TMegamindSessionRequest::kStoreRequest: {
            auto& storeMetrics = metrics->Save;
            storeMetrics.OnStart(startTime);
            storeMetrics.SetSize(req.GetStoreRequest().GetData().size());
            // storeMetrics.SetCompressedSize(NBlockCodecs::Codec("lz4")->Encode(req.GetStoreRequest().GetData()).size());

            try {
                const auto key = ConstructKeyFromRequest(req);
                const TMegamindSessionData data = {
                    .Puid = req.GetStoreRequest().GetPuid(),
                    .Data = req.GetStoreRequest().GetData(),
                };

                logFrame->LogEvent(NEvClass::MMStoreStarted());
                logFrame->LogEvent(NEvClass::RequestKey(key.Key));

                auto storage = SelectStorageForRequest(req.GetStoreRequest());

                SubscribeWithTryCatchWrapper<NEvClass::MMStoreInternalError>(
                    storage->Set(key, data, -1, logFrame),
                    [promise = promise, storeMetrics, startTime, logFrame, key](
                        const NThreading::TFuture<TMegamindSessionStorage::TEmptyResponse>& fut
                    ) mutable {
                        const auto& row = fut.GetValueSync();

                        if (row.Status == EResponseStatus::CREATED) {
                            storeMetrics.OnYdbOk(startTime);
                            logFrame->LogEvent(NEvClass::MMStoreSuccess());
                        } else {
                            storeMetrics.OnYdbError(startTime);
                            logFrame->LogEvent(NEvClass::MMStoreFail(row.Stats.ErrorMessage, row.Status));
                        }

                        NCachalotProtocol::TResponse response;
                        response.SetStatus(row.Status);
                        promise.SetValue(std::move(response));

                        storeMetrics.OnOk(startTime);
                    },
                    logFrame
                );
            } catch (...) {
                const TString& msg = CurrentExceptionMessage();
                storeMetrics.OnInternalError(startTime);
                logFrame->LogEvent(NEvClass::MMStoreInternalError(msg));
                SetException(msg, promise);
            }
            break;
        }
        default: {
            metrics->RequestMetrics.OnBadData(startTime);

            NCachalotProtocol::TResponse response;
            response.SetStatus(EResponseStatus::BAD_REQUEST);
            response.SetStatusMessage("No load or store request found");
            logFrame->LogEvent(NEvClass::MMBadRequest("Uuid unknown"));

            promise.SetValue(std::move(response));
            break;
        }
    };

    return result;
}

NThreading::TFuture<void> TMegamindSessionService::OnApphostRequest(NAppHost::TServiceContextPtr ctx) {
    auto logFrame = MakeIntrusive<TChronicler>();

    TInstant startTime = TInstant::Now();
    DLOG("TMegamindSessionService::OnRequest Started");
    logFrame->LogEvent(NEvClass::MMStart());

    auto promise = NThreading::NewPromise<void>();
    auto result = promise.GetFuture();
    auto* metrics = &TMetrics::GetInstance().MegamindSessionMetrics.Apphost;

    if (!ctx->HasProtobufItem(ITEM_TYPE_REQUEST)) {
        DLOG("TMegamindSessionService::OnRequest No correct protobuf item found");
        metrics->RequestMetrics.OnBadData(startTime);
        logFrame->LogEvent(NEvClass::MMBadRequest("Uuid unknown"));

        NCachalotProtocol::TResponse response;
        response.SetStatus(EResponseStatus::BAD_REQUEST);
        response.SetStatusMessage("No correct protobuf item found");
        ctx->AddProtobufItem(std::move(response), ITEM_TYPE_RESPONSE);

        promise.SetValue();
        return result;
    }

    auto req = ctx->GetOnlyProtobufItem<NCachalotProtocol::TMegamindSessionRequest>(ITEM_TYPE_REQUEST);
    SubscribeWithTryCatchWrapper<NEvClass::MMUnknownError>(
        OnRequest(
            std::move(req), startTime, metrics, logFrame
        ),
        [promise = std::move(promise), ctx, metrics, startTime, logFrame](
            const NThreading::TFuture<NCachalotProtocol::TResponse>& fut
        ) mutable {
            const auto& response = fut.GetValueSync();
            ctx->AddProtobufItem(response, ITEM_TYPE_RESPONSE);
            metrics->RequestMetrics.OnOk(startTime);

            int status = static_cast<int>(response.GetStatus());
            logFrame->LogEvent(NEvClass::MMFinish(status));
            if (status / 100 >= 5) {
                // bad status code
                promise.SetException(TStringBuilder{} << "Got status code " << status);
            } else {
                promise.SetValue();
            }
        },
        logFrame
    );

    return result;
}

TRequestMegamindSession::TRequestMegamindSession(const NNeh::IRequestRef& req, TMegamindSessionService& service)
    : TRequest(req, &TMetrics::GetInstance().MegamindSessionMetrics.Http.RequestMetrics)
    , Service(service)
{
}

TAsyncStatus TRequestMegamindSession::ServeAsync() {
    TInstant startTime = TInstant::Now();
    DLOG("TMegamindSessionService::OnRequest Started");
    LogFrame->LogEvent(NEvClass::MMStart());

    auto* metrics = &TMetrics::GetInstance().MegamindSessionMetrics.Http;

    if (!Request.HasMegamindSessionReq()) {
        DLOG("TMegamindSessionService::OnRequest No correct protobuf item found");
        metrics->RequestMetrics.OnBadData(startTime);
        LogFrame->LogEvent(NEvClass::MMBadRequest("Uuid unknown"));
        SetError(EResponseStatus::BAD_REQUEST, "No correct protobuf item found");
        return NThreading::MakeFuture(TStatus(Status));
    }

    auto promise = NThreading::NewPromise<TStatus>();
    auto result = promise.GetFuture();

    SubscribeWithTryCatchWrapper<NEvClass::MMUnknownError>(
        Service.OnRequest(
            Request.GetMegamindSessionReq(), startTime, metrics, LogFrame
        ),
        [this, promise = std::move(promise), startTime, metrics](
            const NThreading::TFuture<NCachalotProtocol::TResponse>& fut
        ) mutable {
            Response = fut.GetValueSync();
            metrics->RequestMetrics.OnOk(startTime);
            SetStatus(Response.GetStatus());
            LogFrame->LogEvent(NEvClass::MMFinish(static_cast<int>(Response.GetStatus())));
            promise.SetValue(Response.GetStatus());
        },
        LogFrame
    );

    return result;
}

void TMegamindSessionService::OnHttpRequest(const NNeh::IRequestRef& request) {
    TRequestPtr req = MakeIntrusive<TRequestMegamindSession>(request, *this);

    if (IsSuspended()) {
        req->SetError(EResponseStatus::SERVICE_UNAVAILABLE, "Service is suspended");
        req->ReplyTo(request);
        return;
    }

    if (req->IsFinished()) {
        req->ReplyTo(request);
        return;
    }

    req->ServeAsync().Subscribe([request, req](const TAsyncStatus&) {
        req->ReplyTo(request);
    });
}

template <typename TProtoRequest>
TIntrusivePtr<IKvStorage<TMegamindSessionKey, TMegamindSessionData>>
TMegamindSessionService::SelectStorageForRequest(
    const TProtoRequest& request
) const {
    if (request.HasLocation() && (TInstanceTags::Get().Geo != request.GetLocation())) {
        return Storage->GetLocalStorage();
    }
    return Storage;
}

bool TMegamindSessionService::Integrate(NAppHost::TLoop& loop, uint16_t port) {
    loop.Add(port, "/mm_session", [this](NAppHost::TServiceContextPtr ctx) {
        return this->OnApphostRequest(ctx);
    });

    loop.Add(port, "/mm_session_http", [this](const NNeh::IRequestRef& request) {
        this->OnHttpRequest(request);
    });

    return true;
}

}   // namespace NCachalot
