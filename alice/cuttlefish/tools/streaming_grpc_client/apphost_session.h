#pragma once

#include <apphost/lib/grpc/protos/service.grpc.pb.h>
#include <apphost/lib/grpc/client/grpc_pool.h>
#include <library/cpp/threading/future/future.h>
#include <util/generic/queue.h>
#include <util/generic/maybe.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>

#include <apphost/lib/grpc/json/service_response.h>


// Must be used in one thread only

class TAppHostSession : private NAppHost::NTransport::NGrpc::TGrpcPool::TInvokeBase
{
    // - `Write()` and `WritesDone()` must be invoked within the same single thead; it's
    // thread-safe regarding `Read()`
    // - `Read()` must be called within a single thread

    friend class TDefaultIntrusivePtrOps<TAppHostSession>;

public:
    using TServiceRequest = NAppHostProtocol::TServiceRequest;
    using TServiceResponse = NAppHostProtocol::TServiceResponse;
    using TMaybeServiceResponse = TMaybe<TServiceResponse>;

    static TIntrusivePtr<TAppHostSession> Start(NAppHost::NTransport::NGrpc::TGrpcPool& grpcPool, const TString& addr, bool secure = false);

public:
    inline NThreading::TFuture<bool> IsReady() const {
        return IsReadyPromise.GetFuture();
    }


    inline NThreading::TFuture<bool> Write(TServiceRequest&& request) {
        return Send(std::move(request));
    }

    inline NThreading::TFuture<bool> WritesDone() {
        return Send(Nothing());
    }

    // Read
    NThreading::TFuture<TMaybeServiceResponse> Read();

private:
    using TMaybeRequest = TMaybe<TServiceRequest>;

    TAppHostSession()
        : TInvokeBase(NAppHost::NGrpc::NClient::TChannel::EMethod::InvokeEx)
        , IsReadyPromise(NThreading::NewPromise<bool>())
    { }

    virtual void HandleWrite(bool ok) override;
    virtual void HandleWritesDone(bool ok) override;
    virtual void DoHandleRead(bool ok) override;
    virtual void HandleReady(bool ok) override;

    NThreading::TFuture<bool> Send(TMaybeRequest&&);

    NThreading::TPromise<TMaybeServiceResponse> ResponsePromise;
    NThreading::TPromise<bool> IsReadyPromise;
    NThreading::TPromise<bool> WritePromise;
};



//----------------------------------------------------------------------------------------------------------------
