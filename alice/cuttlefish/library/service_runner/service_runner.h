#pragma once

#include <alice/cuttlefish/library/apphost/admin_handle_listener.h>
#include <alice/cuttlefish/library/apphost/metrics_services.h>
#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/metrics/metrics.h>
#include <alice/cuttlefish/library/mlock/mlock.h>
#include <alice/cuttlefish/library/proto_converters/converters.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <apphost/api/service/cpp/service.h>

#include <voicetech/library/evlogdump/evlogdump.h>

#include <contrib/libs/protobuf/src/google/protobuf/text_format.h>

#include <library/cpp/getopt/small/modchooser.h>
#include <library/cpp/neh/http2.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/http_headers.h>
#include <library/cpp/proto_config/config.h>
#include <library/cpp/proto_config/load.h>
#include <library/cpp/proto_config/usage.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/unistat/unistat.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/system/getpid.h>
#include <util/system/hostname.h>


namespace NAlice {

    void InitSolomon(TStringBuf serviceName, bool maskHost);


    // TService_ MUST be class with members:
    //      TConfig -- class type
    //      DefaultConfigResource -- TString (static member)
    //
    //      TRequestProcessor(TService&, TServiceContextPtr ctx, NThreading::TPromise promise)) -- constructor
    //      OnItem(TServiceContextPtr ctx, TStringBuf tag, TStringBuf type, const NAppHost::NService::TProtobufItem&)
    //            return FinishProcesing or IntermediateFlush (enums)
    //      OnClose(TServiceContextPtr ctx)
    //
    template<typename TService_>
    int RunServiceMain(int argc, const char** argv) {
        try {
            using namespace NVoicetech;

            TString filenameConfig = argv[1];
            typename TService_::TConfig config;
            NProtoConfig::TLoadConfigOptions cfgOpt;
            cfgOpt.Resource = TService_::DefaultConfigResource;
            NProtoConfig::GetOpt(argc, argv, config, cfgOpt);
            NCuttlefish::TLogger::TConfig loggerConfig;
            {
                auto log = config.server().log();
                loggerConfig.Debug = log.debug();
                loggerConfig.Filename = log.eventlog();
                loggerConfig.NumThreads = log.num_threads();
                loggerConfig.QueueSize = log.queue_size();
                loggerConfig.EnableDropOnOverflow = log.drop_on_overflow();
            }
            TLogger& logger = NCuttlefish::GetLogger();
            logger.Init(loggerConfig);

            // WARNING: It is important to init metrics before lock memory
            InitSolomon(config.server().metrics().service_name(), config.server().metrics().mask_host());
            NVoice::TryMlockAndReport(config.server().lock_memory(), logger.SpawnFrame());

            // DEBUG dump config
            // TString newProtoString;
            // google::protobuf::TextFormat::PrintToString(config, &newProtoString);
            // Cout << newProtoString << Endl;

            const ui16 port = config.server().http().port();
            const ui16 grpcPort = config.server().grpc().port();
            const size_t appHostThreads = config.server().http().apphost_threads();

            NCuttlefish::TAdminHandleListener adminHandleListener(port);

            NAlice::NCuttlefish::TRtLogClient rtLogClient;
            rtLogClient.Init(config.server().rtlog());

            TService_ service(config);
            NAppHost::TLoop loop;
            loop.SetAdminHandleListener(&adminHandleListener);
            loop.SetMaxQueueSize(config.server().http().max_queue_size());
            loop.EnableGrpc(config.server().grpc().port(), {
                .ReusePort = true,
                .Threads=config.server().grpc().threads(),
            });

            for (const auto path : {TService_::Path(), TString("/_streaming_no_block_outputs") + TService_::Path()}) {
                loop.Add(port, path, NAppHost::TAsyncAppService{[&service, &rtLogClient](auto ctx) {
                    auto promise = NThreading::NewPromise();
                    service.CreateProcessor(*ctx, &rtLogClient)->ProcessInput(ctx, promise);
                    return promise;
                }});
            }

            loop.Add(port, "/unistat", NCuttlefish::GolovanMetricsService);
            loop.Add(config.server().http().port(), "/_metrics", NCuttlefish::SolomonMetricsService);

            TString svnVersion;
            {
                TStringOutput so(svnVersion);
                so << GetArcadiaSourceUrl() << '@' << GetArcadiaLastChange();
            }

            logger.SpawnFrame()->LogEvent(NEvClass::StartAppHostService(HostName(), port, grpcPort, svnVersion, GetPID()));
            loop.Loop(appHostThreads);
            logger.SpawnFrame()->LogEvent(NEvClass::StoppedAppHostService(HostName(), port, grpcPort, svnVersion, GetPID()));
            return 0;
        } catch (...) {
            Cerr << CurrentExceptionMessage() << Endl;
            return 1;
        }
    }
    template<typename TService_>
    int RunService(int argc, const char** argv) {
        TModChooser modChooser;
        modChooser.AddMode("run", RunServiceMain<TService_>, "Run service");
        modChooser.AddMode("evlogdump", EventLogDumpMain, "Run evlogdump");
        modChooser.SetDefaultMode("run");
        return modChooser.Run(argc, argv);
    }
}
