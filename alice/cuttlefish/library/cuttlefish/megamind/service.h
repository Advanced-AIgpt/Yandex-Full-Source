#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>
#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {

    NThreading::TPromise<void> MegamindRun(const NAliceCuttlefishConfig::TConfig& cfg, NAppHost::TServiceContextPtr serviceCtx, TLogContext logContext);
    NThreading::TPromise<void> MegamindApply(const NAliceCuttlefishConfig::TConfig& cfg, NAppHost::TServiceContextPtr serviceCtx, TLogContext logContext);

} // namespace NAlice::NCuttlefish::NAppHostServices
