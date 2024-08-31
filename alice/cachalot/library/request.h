#pragma once

#include <alice/cachalot/library/debug.h>
#include <alice/cachalot/library/golovan.h>
#include <alice/cachalot/library/storage/storage.h>
#include <alice/cachalot/library/status.h>

#include <apphost/api/service/cpp/service_context.h>

#include <library/cpp/neh/rpc.h>


namespace NCachalot {

enum class ERequestFormat {
    Unknown,
    Protobuf,
    Json
};


class TRequest : public TThrRefBase {
private:
    TRequest(TRequestMetrics* requestMetrics);

public:
    TRequest(const NNeh::IRequestRef& req, TRequestMetrics* requestMetrics);
    TRequest(NAppHost::TServiceContextPtr ctx, TRequestMetrics* requestMetrics);

    virtual ~TRequest();

    inline void SetStatus(EResponseStatus status) {
        Status = status;
        Response.SetStatus(status);
    }

    inline void SetError(EResponseStatus status) {
        Status = status;
        Response.SetStatus(status);
    }

    inline void SetErrorMessage(const TString& message) {
        Response.SetStatusMessage(message);
    }

    inline void SetError(EResponseStatus status, const TString& message) {
        SetError(status);
        SetErrorMessage(message);
    }

    inline bool IsFinished() const {
        return Status != EResponseStatus::PENDING;
    }

    NThreading::TFuture<void> GetFinishFuture() {
        return FinishPromise.GetFuture();
    }

    void AddBackendStats(const TString& backend, EResponseStatus status, const TStorageStats& stats);

    virtual TAsyncStatus ServeAsync() = 0;

    void ReplyTo(const NNeh::IRequestRef& req);
    void ReplyTo(NAppHost::TServiceContextPtr ctx, TString requestKey = "");  // Implement ReplyToApphostContextOnSuccess in child class!

protected:
    void LogFinalStatus() const;
    virtual void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx);  // Not implemented!
    virtual void ReplyToApphostContextOnError(NAppHost::TServiceContextPtr ctx, TStringBuf requestKey);
    virtual void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx);  // Not implemented!
    virtual void ReplyToApphostContextOnSuccess(NAppHost::TServiceContextPtr ctx, TStringBuf requestKey);

protected:
    TRequestMetrics*                RequestMetrics;
    ERequestFormat                  RequestFormat;
    EResponseStatus                 Status;
    TInstant                        ArrivalTime;
    TString                         ReqId;
    NCachalotProtocol::TRequest     Request;
    NCachalotProtocol::TResponse    Response;
    TChroniclerPtr                  LogFrame;
    NThreading::TPromise<void>      FinishPromise;
};

using TRequestPtr = TIntrusivePtr<TRequest>;


}   // namespace NCachalot
