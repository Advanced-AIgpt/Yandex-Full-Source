#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>
#include <util/generic/strbuf.h>
#include <apphost/api/service/cpp/service.h>

namespace NAlice::NCuttlefish::NAppHostServices {
    /// @brief replace 100500 sync_state backends to one with static response (for ut without network access)
    void FakeSynchronizeState(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, NAlice::NCuttlefish::TLogContext logContext);
}
