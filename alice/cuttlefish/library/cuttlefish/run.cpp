#include "run.h"

#include <alice/cuttlefish/library/cuttlefish/services.h>

#include <alice/cuttlefish/library/cuttlefish/common/edge_flags.h>
#include <alice/cuttlefish/library/cuttlefish/common/item_types.h>
#include <alice/cuttlefish/library/cuttlefish/common/metrics.h>
#include <alice/cuttlefish/library/cuttlefish/common/utils.h>

#include <alice/cuttlefish/library/apphost/admin_handle_listener.h>
#include <alice/cuttlefish/library/apphost/metrics_services.h>
#include <alice/cuttlefish/library/logging/dlog.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/protos/session.pb.h>

#include <alice/cuttlefish/library/mlock/mlock.h>

#include <alice/protos/api/meta/backend.pb.h>

#include <library/cpp/getopt/last_getopt.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/http_headers.h>
#include <library/cpp/neh/http2.h>

#include <util/generic/guid.h>


using namespace NAlice;
using namespace NAlice::NCuttlefish;
using namespace NAppHost;


namespace {
    void LogRequestStart(
        const TLogContext& logContext,
        const NAlice::NCuttlefish::TServiceHandler& service,
        IServiceContext& ctx
    ) {
        // We want to show this message in setrace regardless of logging options
        TLogContext::TOptions options = logContext.Options();
        options.WriteInfoToRtLog = true;
        options.WriteInfoToEventLog = true;

        TLogContext verboseLogContext(
            logContext.FramePtr(),
            logContext.RtLogPtr(),
            options
        );

        verboseLogContext.LogEventInfoCombo<NEvClass::CuttlefishServiceFrame>(
            TString(service.Type),
            GuidToUuidString(ctx.GetRequestID()),
            ctx.GetRUID(),
            TString(ctx.GetLocation().Path),
            TString(service.Path),
            TString(ctx.GetRemoteHost())
        );
    }

    TLogContext::TOptions ReadLoggerOptions(IServiceContext& ctx) {
        // Default options: all logs are allowed.
        TLogContext::TOptions options;

        options.WriteInfoToRtLog = !ctx.CheckFlagInInputContext(EDGE_FLAG_DO_NOT_WRITE_INFO_TO_RTLOG);
        options.WriteInfoToEventLog = !ctx.CheckFlagInInputContext(EDGE_FLAG_DO_NOT_WRITE_INFO_TO_EVENTLOG);

        if (options.WriteInfoToRtLog && options.WriteInfoToEventLog) {
            // If both options have default value, probably flags were not set at all.
            // So we double check via special item.

            if (ctx.HasProtobufItem(ITEM_TYPE_LOGGER_OPTIONS)) {
                // TODO (paxakor): eliminate duplicated parsing.
                const auto& loggerOptions = ctx.GetOnlyProtobufItem<NAliceProtocol::TLoggerOptions>(
                    ITEM_TYPE_LOGGER_OPTIONS
                );

                options.WriteInfoToRtLog = loggerOptions.GetWriteInfoToRtLog();
                options.WriteInfoToEventLog = loggerOptions.GetWriteInfoToEventLog();
            }

            if (ctx.HasProtobufItem(ITEM_TYPE_ALICE_LOGGER_OPTIONS)) {
                const auto& aliceLoggerOptions = ctx.GetOnlyProtobufItem<NAlice::TLoggerOptions>(
                    ITEM_TYPE_ALICE_LOGGER_OPTIONS
                );

                if (aliceLoggerOptions.GetSetraceLogLevel() <= NAlice::ELogLevel::ELL_INFO) {
                    options.WriteInfoToRtLog = true;
                    options.WriteInfoToEventLog = true;
                }
            }
        }

        return options;
    }

    void PropagateLoggerOptions(IServiceContext& ctx, const TLogContext::TOptions& options) {
        if (!options.WriteInfoToRtLog) {
            ctx.AddFlag(EDGE_FLAG_DO_NOT_WRITE_INFO_TO_RTLOG);
        }
        if (!options.WriteInfoToEventLog) {
            ctx.AddFlag(EDGE_FLAG_DO_NOT_WRITE_INFO_TO_EVENTLOG);
        }
    }

    TLogContext MakeLogContextWithOptions(IServiceContext& ctx, TRtLogClient& rtLogClient) {
        TLogContext::TOptions options = ReadLoggerOptions(ctx);

        // If LoggerOptions was found, we are trying to pass this options as flags to the next nodes.
        // It will work, if god of apphost blesses us.
        PropagateLoggerOptions(ctx, options);

        return TLogContext(
            SpawnLogFrame(/* needAlwaysSafeAdd = */ true),
            rtLogClient.CreateRequestLogger(TryGetRtLogTokenFromAppHostContext(ctx).GetOrElse("")),
            std::move(options)
        );
    }
}

int NAlice::NCuttlefish::Run(const NAliceCuttlefishConfig::TConfig& config) {
    TLogger& logger = GetLogger();
    {
        TLogger::TConfig loggerConfig;
        const auto& log = config.server().log();
        loggerConfig.Debug = log.debug();
        loggerConfig.Filename = log.eventlog();
        loggerConfig.NumThreads = log.num_threads();
        loggerConfig.QueueSize = log.queue_size();
        loggerConfig.EnableDropOnOverflow = log.drop_on_overflow();

        logger.Init(loggerConfig);
    }

    // WARNING: It is important to init metrics before lock memory
    InitMetrics();
    NVoice::TryMlockAndReport(config.server().lock_memory(), logger.SpawnFrame());

    TRtLogClient rtLogClient;
    rtLogClient.Init(config.server().rtlog());

    // NOTE: don't need to reopen RTLog for it does this automatically
    TAdminHandleListener adminHandleListener(static_cast<ui16>(config.server().http().port()));

    TLoop loop;
    loop.SetAdminHandleListener(&adminHandleListener);

    int ret = PrepareLoop(config, loop, rtLogClient);
    if (ret != 0) {
        return ret;  // got fatal error
    }

    DLOG("Start main loop on " << config.server().http().apphost_threads() << " threads");
    try {
        loop.Loop(config.server().http().apphost_threads());
    } catch (...) {
        Cerr << TInstant::Now() << ' ' << CurrentExceptionMessage() << Endl;
        return 1;
    }
    return 0;
}


int PrepareAppHostServices(
    const NAliceCuttlefishConfig::TConfig& config,
    NAppHost::TLoop& loop,
    TRtLogClient& rtLogClient
) {
    const auto services = GetAppHostServices(config);
    for (const auto& service : services) {
        if (service.RequestHandler) {
            loop.Add(config.server().http().port(), TString(service.Path), [service, &rtLogClient](IServiceContext& ctx) {
                TLogContext logContext = MakeLogContextWithOptions(ctx, rtLogClient);
                LogRequestStart(logContext, service, ctx);

                try {
                    service.RequestHandler(ctx, logContext);
                    logContext.LogEventInfoCombo<NEvClass::InfoMessage>("Finished");
                } catch (...) {
                    logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Failed: " << CurrentExceptionMessage());
                    throw;
                }
            });
        } else if (service.StreamHandler) {
            loop.Add(config.server().http().port(), TString(service.Path), NAppHost::TAsyncAppService{[service, &rtLogClient](auto ctx) {
                TLogContext logContext = MakeLogContextWithOptions(*ctx, rtLogClient);
                LogRequestStart(logContext, service, *ctx);

                try {
                    auto promise = service.StreamHandler(ctx, logContext);
                    logContext.LogEventInfoCombo<NEvClass::InfoMessage>("Finished with first chunk");
                    return promise;
                } catch (...) {
                    logContext.LogEventErrorCombo<NEvClass::ErrorMessage>(TStringBuilder() << "Failed: " << CurrentExceptionMessage());
                    throw;
                }
            }});
        } else {
            Cerr << "unknown service type" << Endl;
            return 1;
        }
    }
    return 0;
}


int PrepareHttpServices(const NAliceCuttlefishConfig::TConfig& config, NAppHost::TLoop& loop) {
    loop.Add(config.server().http().port(), "/_metrics", SolomonMetricsService);

    return 0;
}


int NAlice::NCuttlefish::PrepareLoop(
    const NAliceCuttlefishConfig::TConfig& config,
    NAppHost::TLoop& loop,
    TRtLogClient& rtLogClient
) {
    loop.SetMaxQueueSize(config.server().http().max_queue_size());
    loop.EnableGrpc(
        config.server().grpc().port(),
        {
            .ReusePort = true,
            .Threads = config.server().grpc().threads(),
        }
    );
    DLOG("gRPC is enabled on " << config.server().grpc().port() << " port");

    if (const int ret = PrepareAppHostServices(config, loop, rtLogClient)) {
        return ret;
    }

    if (const int ret = PrepareHttpServices(config, loop)) {
        return ret;
    }

    return 0;
}
