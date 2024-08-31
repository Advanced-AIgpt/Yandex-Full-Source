#include "subsystem_logging.h"

#include <alice/cuttlefish/library/logging/event_log.h>
#include <alice/cuttlefish/library/proto_configs/rtlog.cfgproto.pb.h>


namespace NGProxy {


void TLoggingSubsystem::Init() {
    InitEventLog();
    InitRtLog();
}


void TLoggingSubsystem::InitEventLog() {
    NAlice::NCuttlefish::TLogger::TConfig config;
    config.Filename = Config_.GetEventLogPath();
    NAlice::NCuttlefish::GetLogger().Init(config);
}


void TLoggingSubsystem::InitRtLog() {
    NAliceServiceConfig::RtLog cfg;
    cfg.set_file(Config_.GetRtLogPath());
    cfg.set_service("gproxy");

    if (Config_.GetUnifiedAgentLogPath() && Config_.GetUnifiedAgentUri()) {
        cfg.set_unified_agent_log_file(Config_.GetUnifiedAgentLogPath());
        cfg.set_unified_agent_uri(Config_.GetUnifiedAgentUri());
    }

    NAlice::NCuttlefish::TRtLogClient::Instance().Init(cfg);
}


}   // namespace NGProxy
