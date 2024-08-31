#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>
#include <alice/cuttlefish/library/proto_configs/cuttlefish.cfgproto.pb.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/strbuf.h>


namespace NAlice::NCuttlefish::NAppHostServices {

void BioContextSavePre(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext);
void BioContextSavePost(const NAliceCuttlefishConfig::TConfig& config, NAppHost::IServiceContext& serviceCtx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices
