#include <alice/cachalot/library/modules/gdpr/request.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <util/generic/set.h>

namespace NCachalot {

TRequestGDPR::TRequestGDPR(const NNeh::IRequestRef& req, TIntrusivePtr<IGDPRStorage> storage)
    : TRequest(req, &TMetrics::GetInstance().GDPRMetrics.RequestMetrics)
    , Storage(std::move(storage))
{
}

TAsyncStatus TRequestGDPR::ServeAsync() {
    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));

    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    if (!Storage) {
        SetError(EResponseStatus::INTERNAL_ERROR, "Internal Error [Storage=Null]");
        return NThreading::MakeFuture(TStatus(Status));
    }


    if (Request.HasGDPRGetReq()) {
        const auto& getReq = Request.GetGDPRGetReq();
        TGDPRKey key;
        key.Puid = getReq.GetKey().GetPuid();
        key.ComputeShardKey();

        auto callback = [this, status, This=IntrusiveThis()](const TAsyncGDPRGetUserDataResponse& reply) mutable {
            const TSingleRowResponse& r = reply.GetValueSync();
            if (r.Status != EResponseStatus::OK) {
                SetStatus(r.Status);
                status.SetValue(r);
                return;
            }
            const TGDPRGetUserDataResponse& resp = *r.Data;
            ::NCachalotProtocol::TGDPRGetUserDataResponse* response = Response.MutableGDPRGetUserDataResp();
            ::NCachalotProtocol::TGDPRGetSuccess* success = response->MutableSuccess();
            ::NCachalotProtocol::TGDPRData* statuses = success->MutableData();
            for (const auto& serviceStatus : resp.Data.Statuses) {
                ::NCachalotProtocol::TGDPRStatus* gdprStatus = statuses->AddStatuses();
                gdprStatus->SetService(serviceStatus.Service);
                gdprStatus->SetStatus(serviceStatus.Status);
                gdprStatus->SetTimestamp(serviceStatus.Timestamp);
            }
            status.SetValue(EResponseStatus::OK);
        };

        Storage->GetUserData(key).Subscribe(callback);

    } else if (Request.HasGDPRSetReq()) {
        const auto& setReq = Request.GetGDPRSetReq();

        TGDPRKey key;
        key.Puid = setReq.GetKey().GetPuid();
        key.ComputeShardKey();

        auto callback = [this, status, key, This=IntrusiveThis()](const TAsyncGDPRSetUserDataResponse& reply) mutable {
            const TSingleRowSetResponse& r = reply.GetValueSync();
            if (r.Status != EResponseStatus::OK) {
                SetStatus(r.Status);
                status.SetValue(r);
                return;
            }
            ::NCachalotProtocol::TGDPRSetUserDataResponse* response = Response.MutableGDPRSetUserDataResp();

            ::NCachalotProtocol::TGDPRSetSuccess* success = response->MutableSuccess();
            ::NCachalotProtocol::TGDPRKey* gdprKey = success->MutableKey();
            gdprKey->SetPuid(key.Puid);
            status.SetValue(EResponseStatus::OK);
        };

        TSet<TString> services = {"Memento", "YabioContext", "VinsContext", "Datasync", "Logs", "Notificator"};
        const auto& gdprStatus = setReq.GetStatus();
        if (services.find(gdprStatus.GetService()) == services.end()) {
            ::NCachalotProtocol::TGDPRSetUserDataResponse* response = Response.MutableGDPRSetUserDataResp();
            ::NCachalotProtocol::TGDPRError* error = response->MutableError();
            error->SetText("Unknown service \"" + gdprStatus.GetService() + "\"");
            SetStatus(EResponseStatus::BAD_REQUEST);
            status.SetValue(EResponseStatus::BAD_REQUEST);
            return status;
        }
        TGDPRData data;
        data.Statuses.push_back({gdprStatus.GetService(), gdprStatus.GetStatus(), gdprStatus.GetTimestamp()});
        Storage->SetUserData(key, data).Subscribe(callback);

    } else if (Request.HasGDPRGetRequestsReq()) {
        const auto& getRequestsReq = Request.GetGDPRGetRequestsReq();

        int limit = getRequestsReq.GetLimit();
        int offset = getRequestsReq.GetOffset();

        auto callback = [this, status, This=IntrusiveThis()](const TAsyncGDPRGetRequestsResponse& reply) mutable {
            const TGDPRGetRequestsResponse& r = *reply.GetValueSync().Data;
            ::NCachalotProtocol::TGDPRGetRequestsResponse* response = Response.MutableGDPRGetRequestsResp();
            ::NCachalotProtocol::TGDPRGetRequestsSuccess* success = response->MutableSuccess();
            for (const auto& kv : r.PersonalizedData) {
                ::NCachalotProtocol::TGDPRPersonalizedData* request = success->AddData();
                // kv = {Puid, TGDPRData}
                request->SetPuid(kv.first);
                for (const auto& serviceStatus : kv.second.Statuses) {
                    ::NCachalotProtocol::TGDPRData* data = request->MutableData();
                    ::NCachalotProtocol::TGDPRStatus* gdprStatus = data->AddStatuses();
                    gdprStatus->SetService(serviceStatus.Service);
                    gdprStatus->SetStatus(serviceStatus.Status);
                    gdprStatus->SetTimestamp(serviceStatus.Timestamp);
                }
            }
            success->SetLimit(r.Limit);
            success->SetOffset(r.Offset);
            status.SetValue(EResponseStatus::OK);
        };

        Storage->GetRequests(limit, offset).Subscribe(callback);
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Invalid request [unknown request type]");
        return NThreading::MakeFuture(TStatus(Status));
    }
    SetStatus(EResponseStatus::OK);
    return status;
}

}   // namespace NCachalot
