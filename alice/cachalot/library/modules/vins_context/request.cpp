#include <alice/cachalot/library/modules/vins_context/request.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <util/generic/set.h>

namespace NCachalot {

TRequestVinsContext::TRequestVinsContext(
    const NNeh::IRequestRef& req,
    TIntrusivePtr<IKvStorage<TVinsContextKey, TString, TVinsExtraData>> storage
)
    : TRequest(req, TMetrics::GetInstance().VinsContextMetrics())
    , Storage(std::move(storage))
{
}

TAsyncStatus TRequestVinsContext::ServeAsync() {
    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));

    if (!Storage) {
        SetError(EResponseStatus::INTERNAL_ERROR, "Internal Error [Storage=Null]");
        return NThreading::MakeFuture(TStatus(Status));
    }

    if (!Request.HasVinsContextReq()) {
        SetError(EResponseStatus::BAD_REQUEST, "Bad Request [VinsContextReq=Null]");
        return NThreading::MakeFuture(TStatus(Status));
    }

    const auto& req = Request.GetVinsContextReq();

    /*
    if (req.HasSave()) {
        SetError(EResponseStatus::INTERNAL_ERROR, "Internal Error [Method Save is not implemented yet]");
        return NThreading::MakeFuture(TStatus(Status));
    }

    if (req.HasLoad()) {
        SetError(EResponseStatus::INTERNAL_ERROR, "Internal Error [Method Load is not implemented yet]");
        return NThreading::MakeFuture(TStatus(Status));
    }
    */

    if (req.HasDelete()) {
        const auto& deleteReq = req.GetDelete();
        const TVinsContextKey key = deleteReq.GetKey();

        NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

        auto callback = [this, status, This=IntrusiveThis()](const NThreading::TFuture<TEmptyResponse>& reply) mutable {
            auto result = reply.GetValueSync();
            if (!result) {
                Cerr << "Got Resp: some bad " << result.Stats.ErrorMessage << Endl;
                SetStatus(result.Status);
                status.SetValue(result);
                return;
            }
            ::NCachalotProtocol::TVinsContextResponse* response = Response.MutableVinsContextResp();
            ::NCachalotProtocol::TVinsContextSuccess* success = response->MutableSuccess();

            success->SetOk(true);
            success->SetNumDeleted(result.ExtraData.NumDeletedRows);
            status.SetValue(EResponseStatus::OK);
        };

        Storage->Del(key).Subscribe(callback);
        SetStatus(EResponseStatus::OK);
        return status;
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Invalid request [unknown request type]");
        return NThreading::MakeFuture(TStatus(Status));
    }
}

}   // namespace NCachalot
