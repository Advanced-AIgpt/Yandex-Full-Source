#include <alice/cachalot/library/modules/yabio_context/request.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cachalot/events/cachalot.ev.pb.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>


using namespace NCachalotProtocol;
using namespace NAlice::NCuttlefish;


namespace NCachalot {


TRequestYabioContext::TRequestYabioContext(
    const NNeh::IRequestRef& req,
    TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> storage
)
    : TRequest(req, &(TMetrics::GetInstance().YabioContextServiceMetrics.Request))
    , Storage(std::move(storage))
{
}

TRequestYabioContext::TRequestYabioContext(
    NAppHost::TServiceContextPtr ctx,
    TIntrusivePtr<IKvStorage<TYabioContextKey, TString>> storage
)
    : TRequest(ctx, &(TMetrics::GetInstance().YabioContextServiceMetrics.Request))
    , Storage(std::move(storage))
{
    if (!ctx->HasProtobufItem(ITEM_TYPE_YABIO_CONTEXT_REQUEST)) {
        const TString error = TStringBuilder() << "Input item " << ITEM_TYPE_YABIO_CONTEXT_REQUEST << " is invalid";
        SetError(EResponseStatus::BAD_REQUEST, error);
        LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextBadRequest>(error);
    } else {
        try {
            Request.MutableYabioContextReq()->CopyFrom(
                ctx->GetOnlyProtobufItem<TYabioContextRequest>(ITEM_TYPE_YABIO_CONTEXT_REQUEST)
            );
        } catch (...) {
            const TString error = TStringBuilder() << "Input item " << ITEM_TYPE_YABIO_CONTEXT_REQUEST << " is invalid";
            SetError(EResponseStatus::BAD_REQUEST, error);
            LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextBadRequest>(error);
        }
    }

}

TAsyncStatus TRequestYabioContext::ServeAsync() {
    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));

    if (!Storage) {
        SetError(EResponseStatus::INTERNAL_ERROR, "Internal Error [Storage=Null]");
        return NThreading::MakeFuture(TStatus(Status));
    }

    if (!Request.HasYabioContextReq()) {
        SetError(EResponseStatus::BAD_REQUEST, "Bad Request [YabioContextReq=Null]");
        return NThreading::MakeFuture(TStatus(Status));
    }

    const TYabioContextRequest& req = Request.GetYabioContextReq();

    if (req.HasSave()) {
        return ServeAsyncSave();
    }

    if (req.HasLoad()) {
        return ServeAsyncLoad();
    }

    if (req.HasDelete()) {
        return ServeAsyncDelete();
    }

    SetError(EResponseStatus::BAD_REQUEST, "Invalid request [unknown request type]");
    return NThreading::MakeFuture(TStatus(Status));
}

TAsyncStatus TRequestYabioContext::ServeAsyncSave() {
    Y_ASSERT(Request.HasYabioContextReq());
    Y_ASSERT(Request.GetYabioContextReq().HasSave());

    const TYabioContextSave& req = Request.GetYabioContextReq().GetSave();
    const TYabioContextKey key(req.GetKey());
    LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextSaveBegin>(key.AsString(), req.GetContext().size());

    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    auto callback = [
        this, status, This=IntrusiveThis()
    ](
        const NThreading::TFuture<TYabioContextYdbStorage::TEmptyResponse>& reply
    ) mutable {
        auto result = reply.GetValueSync();
        ::NCachalotProtocol::TYabioContextResponse* response = Response.MutableYabioContextResp();

        if (!result) {
            LogFrame->Log().LogEventErrorCombo<NEvClass::YabioContextSaveError>(
                int(result.Status), result.Stats.ErrorMessage
            );
        } else {
            LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextSaveSuccess>();
            response->MutableSuccess()->SetOk(true);
        }

        SetStatus(result.Status);
        status.SetValue(result);
    };

    Storage->Set(key, req.GetContext(), -1, LogFrame).Subscribe(std::move(callback));
    return status;
}

TAsyncStatus TRequestYabioContext::ServeAsyncLoad() {
    Y_ASSERT(Request.HasYabioContextReq());
    Y_ASSERT(Request.GetYabioContextReq().HasLoad());

    const TYabioContextLoad& req = Request.GetYabioContextReq().GetLoad();
    const TYabioContextKey key(req.GetKey());
    LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextLoadBegin>(key.AsString());

    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    auto callback = [
        this, status, This=IntrusiveThis()
    ](
        const NThreading::TFuture<TYabioContextYdbStorage::TSingleRowResponse>& reply
    ) mutable {
        auto result = reply.GetValueSync();
        ::NCachalotProtocol::TYabioContextResponse* response = Response.MutableYabioContextResp();

        if (!result) {
            LogFrame->Log().LogEventErrorCombo<NEvClass::YabioContextLoadError>(
                int(result.Status), result.Stats.ErrorMessage
            );
            ::NCachalotProtocol::TYabioContextError* error = response->MutableError();
            error->SetText(result.Stats.ErrorMessage);
            error->SetStatus(result.Status);
        } else {
            ::NCachalotProtocol::TYabioContextSuccess* success = response->MutableSuccess();
            if (result.Status == EResponseStatus::OK) {
                Y_ASSERT(result.Data.Defined());
                success->SetOk(true);
                success->SetContext(*result.Data);
                LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextLoadSuccess>(success->GetContext().size());
            } else {
                LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextLoadSuccess>(0);
            }
        }

        SetStatus(result.Status);
        status.SetValue(result);
    };

    Storage->GetSingleRow(key, LogFrame).Subscribe(std::move(callback));
    return status;
}

TAsyncStatus TRequestYabioContext::ServeAsyncDelete() {
    Y_ASSERT(Request.HasYabioContextReq());
    Y_ASSERT(Request.GetYabioContextReq().HasDelete());

    const TYabioContextDelete& req = Request.GetYabioContextReq().GetDelete();
    const TYabioContextKey key(req.GetKey());
    LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextDelBegin>(key.AsString());

    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    auto callback = [
        this, status, This=IntrusiveThis()
    ](
        const NThreading::TFuture<TYabioContextYdbStorage::TEmptyResponse>& reply
    ) mutable {
        auto result = reply.GetValueSync();
        ::NCachalotProtocol::TYabioContextResponse* response = Response.MutableYabioContextResp();

        if (!result) {
            LogFrame->Log().LogEventErrorCombo<NEvClass::YabioContextDelError>(
                int(result.Status), result.Stats.ErrorMessage
            );
            response->MutableError()->SetText(result.Stats.ErrorMessage);
            response->MutableError()->SetStatus(result.Status);
        } else {
            LogFrame->Log().LogEventInfoCombo<NEvClass::YabioContextDelSuccess>();
            response->MutableSuccess()->SetOk(true);
        }

        SetStatus(result.Status);
        status.SetValue(result);
    };

    Storage->Del(key, LogFrame).Subscribe(std::move(callback));
    return status;
}

void TRequestYabioContext::ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx) {
    ctx->AddProtobufItem(Response.GetYabioContextResp(), ITEM_TYPE_YABIO_CONTEXT_RESPONSE);
}

void TRequestYabioContext::ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx) {
    ctx->AddProtobufItem(Response.GetYabioContextResp(), ITEM_TYPE_YABIO_CONTEXT_RESPONSE);
}


}   // namespace NCachalot
