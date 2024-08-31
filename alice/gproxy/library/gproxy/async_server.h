#pragma once

#include <alice/gproxy/library/gproxy/grpc.h>
#include <alice/gproxy/library/gproxy/async_service.h>
#include <alice/gproxy/library/gproxy/config.pb.h>


namespace NGProxy {

class TAsyncServer {
public:
    TAsyncServer(const TGrpcServerConfig& config, TLoggingSubsystem& logging, TMetricsSubsystem& metrics, TAppHostSubsystem& apphost);

    void BuildAndStart();

    void Wait();

//private:
    TGrpcServerConfig   Config_;
    TLoggingSubsystem&  Logging_;
    TMetricsSubsystem&  Metrics_;
    TAppHostSubsystem&  AppHost_;
    TAsyncService       Service_;
    TCompletionQueuePtr Cq_;
    TGrpcServerPtr      Server_;
};

}   // namespace NGProxy
