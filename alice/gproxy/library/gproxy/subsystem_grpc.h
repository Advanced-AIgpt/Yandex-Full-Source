#pragma once

#include <util/generic/ptr.h>

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>

#include "subsystem_logging.h"
#include "subsystem_metrics.h"
#include "subsystem_apphost.h"

#include <alice/gproxy/library/gproxy/async_server.h>
#include <alice/gproxy/library/protos/service.gproxy.pb.h>


namespace NGProxy {

class TGrpcSubsystem {
public:
    TGrpcSubsystem(const TGrpcServerConfig& config, TLoggingSubsystem& logging, TMetricsSubsystem& metrics, TAppHostSubsystem& apphost)
        : AsyncServer(config, logging, metrics, apphost)
    { }

    void Init() {
        AsyncServer.BuildAndStart();
    }

    void Wait() {
        AsyncServer.Wait();
    }

    void Stop() {
    }

private:
    TAsyncServer    AsyncServer;
};  // class TGrpcSubsystem

}   // namespace NGProxy
