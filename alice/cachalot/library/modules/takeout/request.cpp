#include <alice/cachalot/library/modules/takeout/request.h>

#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/utils.h>

#include <alice/cachalot/events/cachalot.ev.pb.h>

#include <util/generic/set.h>

namespace NCachalot {

TRequestTakeout::TRequestTakeout(const NNeh::IRequestRef& req, TIntrusivePtr<ITakeoutResultsStorage> storage)
    : TRequest(req, &TMetrics::GetInstance().TakeoutMetrics.RequestMetrics)
    , Storage(std::move(storage))
{
}

TAsyncStatus TRequestTakeout::ServeAsync() {
    RequestMetrics->OnServeStarted(ArrivalTime);
    Response.MutableStats()->SetWaitingTime(MillisecondsSince(ArrivalTime));

    NThreading::TPromise<TStatus> status = NThreading::NewPromise<TStatus>();

    if (!Storage) {
        SetError(EResponseStatus::INTERNAL_ERROR, "Internal Error [Storage=Null]");
        return NThreading::MakeFuture(TStatus(Status));
    }


    if (Request.HasTakeoutGetResultsReq()) {
        const auto& getReq = Request.GetTakeoutGetResultsReq();
        TTakeoutResultsKey key;
        key.JobId = getReq.GetJobId();
        key.ComputeShardKey();

        LogFrame->LogEvent(getReq);

        auto callback = [this, status, This=IntrusiveThis()](const TAsyncTakeoutGetResultsResponse& reply) mutable {
            const TSingleRowTakeoutGetResponse& r = reply.GetValueSync();
            if (r.Status != EResponseStatus::OK) {
                SetStatus(r.Status);
                status.SetValue(r);
                return;
            }
            const TTakeoutGetResultsResponse& resp = *r.Data;
            ::NCachalotProtocol::TTakeoutGetResultsResponse* response = Response.MutableTakeoutGetResultsResp();
            ::NCachalotProtocol::TTakeoutGetResultsSuccess* success = response->MutableSuccess();
            success->SetPuid(resp.Puid);
            size_t textsNum = 0;
            size_t textsLength = 0;
            for (const auto& text : resp.Texts) {
                success->AddTexts(text);
                textsNum += 1;
                textsLength += text.size();
            }
            TMetrics::GetInstance().TakeoutMetrics.OnTextsSelect(textsNum, textsLength);

            status.SetValue(EResponseStatus::OK);
        };

        Storage->GetResults(key, getReq.GetLimit(), getReq.GetOffset()).Subscribe(std::move(callback));
    } else if (Request.HasTakeoutSetResultsReq()) {
        const auto& setReq = Request.GetTakeoutSetResultsReq();

        auto callback = [this, status, This=IntrusiveThis()](const TAsyncTakeoutSetResultsResponse& reply) mutable {
            const TSingleRowTakeoutSetResponse& r = reply.GetValueSync();
            if (r.Status != EResponseStatus::OK) {
                SetStatus(r.Status);
                status.SetValue(r);
                return;
            }
            status.SetValue(EResponseStatus::OK);
        };

        TTakeoutResults results;
        for (const auto& res : setReq.GetResults()) {
            TTakeoutResult result;
            result.JobId = res.GetJobId();
            result.Puid = res.GetPuid();
            size_t textsNum = 0;
            size_t textsLength = 0;
            for (const auto& text : res.GetTexts()) {
                result.Texts.push_back(text);
                textsNum += 1;
                textsLength += text.size();
            }
            results.Results.push_back(result);
            LogFrame->LogEvent(NEvClass::RequestKey(result.JobId));
            TMetrics::GetInstance().TakeoutMetrics.OnTextsInsert(textsNum, textsLength);
        }

        Storage->SetResults(results).Subscribe(std::move(callback));
    } else {
        SetError(EResponseStatus::BAD_REQUEST, "Invalid request [unknown request type]");
        return NThreading::MakeFuture(TStatus(Status));
    }
    SetStatus(EResponseStatus::OK);
    return status;
}

}   // namespace NCachalot
