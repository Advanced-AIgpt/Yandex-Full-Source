#pragma once

#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>
#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/rtlog.cfgproto.pb.h>
#include <alice/gproxy/library/gproxy/config.pb.h>


namespace NGProxy {

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

    inline TLogContext CreateLogContext(const TString& token = "", bool needAlwaysSafeAdd = false) {
        auto options = TLogContext::TOptions();
        options.WriteInfoToEventLog = Config_.GetWriteInfoToEventLog();
        options.WriteInfoToRtLog = Config_.GetWriteInfoToRtLog();
        return TLogContext(
            NAlice::NCuttlefish::SpawnLogFrame(needAlwaysSafeAdd),
            NAlice::NCuttlefish::TRtLogClient::Instance().CreateRequestLogger(token),
            options);
    }

public: /* logging api */

private:    /* methods */
    void InitEventLog();
    void InitRtLog();

private:
    TLoggingConfig Config_;
};


}   // namespace NGProxy
