#pragma once

#include <alice/cuttlefish/library/cuttlefish/config/config.h>

#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>
#include <alice/cuttlefish/library/rtlog/rtlog.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish {
    int Run(const NAliceCuttlefishConfig::TConfig& config);
    // method for using from unit tests
    int PrepareLoop(
        const NAliceCuttlefishConfig::TConfig& config,
        NAppHost::TLoop& loop,
        TRtLogClient& rtLogClient
    );
}
