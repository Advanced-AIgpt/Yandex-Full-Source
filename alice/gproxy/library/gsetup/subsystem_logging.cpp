#include "subsystem_logging.h"

#include <alice/cuttlefish/library/logging/event_log.h>

namespace NGProxy {


void TLoggingSubsystem::Init() {
    NAlice::NCuttlefish::TLogger::TConfig config;
    config.Filename = Config_.GetEventLogPath();
    NAlice::NCuttlefish::GetLogger().Init(config);
    RtLogConfig_.Setservice("gproxy");
    RtLogConfig_.Setfile(Config_.GetRtLogPath());

    if (Config_.GetUnifiedAgentLogPath() && Config_.GetUnifiedAgentUri()) {
        RtLogConfig_.Setunified_agent_log_file(Config_.GetUnifiedAgentLogPath());
        RtLogConfig_.Setunified_agent_uri(Config_.GetUnifiedAgentUri());
    }

    RtLogClient_.Init(RtLogConfig_);
}


}   // namespace NGProxy
