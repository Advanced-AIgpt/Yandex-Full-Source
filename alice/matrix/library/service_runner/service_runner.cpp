#include "service_runner.h"

#include <alice/cuttlefish/library/proto_configs/rtlog.cfgproto.pb.h>

#include <library/cpp/neh/http2.h>
#include <library/cpp/neh/neh.h>


namespace NMatrix::NApplication {

namespace NPrivate {

void ConfigureLogger(const TLoggerSettings& config) {
    TLogger::TConfig loggerConfig;
    loggerConfig.Filename = config.GetFilePath();
    GetLogger().Init(loggerConfig);
}

void ConfigureNeh(const TNehSettings& config) {
    NNeh::THttp2Options::TcpKeepAlive = config.GetTcpKeepAlive();

    for (const auto& protocolOption : config.GetProtocolOptions()) {
        if (!NNeh::SetProtocolOption(protocolOption.GetKey(), protocolOption.GetValue())) {
            ythrow yexception() << "no such option: " << protocolOption.GetKey();
        }
    }

    {
        const auto& limits = config.GetHttpInputConnectionsLimits();
        NNeh::SetHttp2InputConnectionsLimits(limits.GetSoftLimit(), limits.GetHardLimit());
    }

    {
        const auto& limits = config.GetHttpOutputConnectionsLimits();
        NNeh::SetHttp2OutputConnectionsLimits(limits.GetSoftLimit(), limits.GetHardLimit());
    }
}

NAppHost::TLoop& ConfigureLoop(
    NAppHost::TLoop& loop,
    const TServerSettings& config
) {
    loop.EnableGrpc(
        config.GetGrpcPort(),
        {
            .ReusePort = true,
            .Threads = static_cast<int>(config.GetGrpcThreads()),
        }
    );

    loop.SetAdminThreadCount(config.GetAdminThreads());
    loop.SetToolsThreadCount(config.GetToolsThreads());

    return loop;
}

TRtLogClient CreateRtLogClient(const TRtLogSettings& config) {
    NAliceServiceConfig::TRtLog rtLogConfig;
    if (config.GetFilePath()) {
        rtLogConfig.set_file(config.GetFilePath());
    }
    if (config.GetService()) {
        rtLogConfig.set_service(config.GetService());
    }
    if (config.HasFlushPeriod()) {
        rtLogConfig.set_flush_period(FromString<TDuration>(config.GetFlushPeriod()));
    }
    if (config.HasFileStatCheckPeriod()) {
        rtLogConfig.set_file_stat_check_period(FromString<TDuration>(config.GetFileStatCheckPeriod()));
    }
    if (config.HasFlushSize()) {
        rtLogConfig.set_flush_size(config.GetFlushSize());
    }

    TRtLogClient rtLogClient;
    rtLogClient.Init(rtLogConfig);

    return rtLogClient;
}

NYdb::TDriver CreateYDBDriver(const TYDBClientCommonSettings& config) {
    auto driverConfig = NYdb::TDriverConfig()
        .SetEndpoint(config.GetAddress())
        .SetDatabase(config.GetDBName())
        .SetAuthToken(GetEnv("YDB_TOKEN"))
        .SetBalancingPolicy(NYdb::EBalancingPolicy::UsePreferableLocation)
        .SetNetworkThreadsNum(config.GetNetworkThreads())
        .SetDiscoveryMode(NYdb::EDiscoveryMode::Async);

    return NYdb::TDriver(driverConfig);
}

} // NPrivate

} // namespace NMatrix::NApplication
