#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/rtlog.cfgproto.pb.h>
#include <alice/gproxy/library/gsetup/config.pb.h>
#include <alice/library/logger/logger.h>


namespace NGProxy {

using namespace NGSetup;

using TLogContext = NAlice::NCuttlefish::TLogContext;


class TLoggingSubsystem {
public:
    TLoggingSubsystem(const TLoggingConfig& config)
        : Config_(config)
    { }

public: /* subsystem api */
    void Init();

    inline void Wait() { }

    inline void Stop() { }

    inline TLogContext CreateLogContext(NAppHost::IServiceContext& ctx, bool needAlwaysSafeAdd = false) {
        auto options = TLogContext::TOptions();
        options.WriteInfoToEventLog = Config_.GetWriteInfoToEventLog();
        options.WriteInfoToRtLog = Config_.GetWriteInfoToRtLog();
        return TLogContext(
            NAlice::NCuttlefish::SpawnLogFrame(needAlwaysSafeAdd),
            RtLogClient_.CreateRequestLogger(NAlice::NCuttlefish::TryGetRtLogTokenFromAppHostContext(ctx).GetOrElse("")),
            options);
    }

public: /* logging api */

private:
    TLoggingConfig Config_;
    NAlice::NCuttlefish::TRtLogClient RtLogClient_;
    NAliceServiceConfig::RtLog RtLogConfig_;
};


}   // namespace NGProxy
