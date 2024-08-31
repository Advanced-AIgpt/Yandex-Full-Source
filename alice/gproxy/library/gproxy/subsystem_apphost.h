#pragma once

#include <apphost/api/client/client.h>

#include <alice/gproxy/library/gproxy/subsystem_logging.h>
#include <alice/gproxy/library/gproxy/subsystem_metrics.h>
#include <alice/gproxy/library/gproxy/config.pb.h>


namespace NGProxy {

class TAppHostSubsystem {
public:
    TAppHostSubsystem(const TAppHostClientConfig& config, TLoggingSubsystem&, TMetricsSubsystem&);

    void Init();
    void Wait();
    void Stop();

    NAppHost::NClient::TStream CreateStream(TStringBuf vertical, TStringBuf graph, int64_t timeout = 1000);

private:
    void AddBackendForVertical(const TVerticalConfig& vertical);

private:
    TAppHostClientConfig Config_;
    NAppHost::NClient::TClient Client;
};

}   // namespace NGProxy
