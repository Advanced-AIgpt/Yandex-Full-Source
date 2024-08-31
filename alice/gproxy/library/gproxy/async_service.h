#pragma once

#include <alice/gproxy/library/gproxy/subsystem_logging.h>
#include <alice/gproxy/library/gproxy/subsystem_metrics.h>
#include <alice/gproxy/library/gproxy/subsystem_apphost.h>

#include <alice/gproxy/library/gproxy/grpc.h>

#include <alice/gproxy/library/protos/service.grpc.pb.h>


namespace NGProxy {

class TAsyncService {
public:
    TAsyncService(TLoggingSubsystem& logging, TMetricsSubsystem &metrics, TAppHostSubsystem& apphost)
        : Logging_(logging)
        , Metrics_(metrics)
        , AppHost_(apphost)
    { }

    NGProxy::GProxy::AsyncService* Construct(TCompletionQueuePtr& queue);

    NGProxy::GProxy::AsyncService* Get() {
        return &AsyncService_;
    }

    TAppHostSubsystem& GetAppHost() {
        return AppHost_;
    }

    TMetricsSubsystem& GetMetrics() {
        return Metrics_;
    }

    grpc::ServerCompletionQueue* GetQueue() {
        return CompletionQueue_.get();
    }

    void SetQueue(TCompletionQueuePtr&& ptr) {
        CompletionQueue_ = std::move(ptr);
    }

    TLoggingSubsystem& GetLogger() {
        return Logging_;
    }

    void BuildAndStart();

    void DoRpcs();

    inline bool GetAllowSrcrwr() const {
        return AllowSrcrwr_;
    }

    inline void SetAllowSrcrwr(bool allow) {
        AllowSrcrwr_ = allow;
    }

    inline bool GetAllowDumpRequestsResponses() const {
        return AllowDumpReqResp_;
    }

    inline void SetAllowDumpRequestsResponses(bool allow) {
        AllowDumpReqResp_ = allow;
    }
//private:
    TLoggingSubsystem& Logging_;
    TMetricsSubsystem& Metrics_;
    TAppHostSubsystem& AppHost_;

private:
    NGProxy::GProxy::AsyncService  AsyncService_;
    TCompletionQueuePtr            CompletionQueue_;
    bool                           AllowSrcrwr_ { false };
    bool                           AllowDumpReqResp_ { false };
};

}   // namespace NGProxy
