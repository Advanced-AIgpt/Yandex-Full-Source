#pragma once

#include <alice/cuttlefish/library/logging/log_context.h>

#include <apphost/api/service/cpp/service.h>

#include <util/generic/strbuf.h>


namespace NAlice::NCuttlefish::NAppHostServices {

void ContextSavePre(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);
void ContextSavePost(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);
void FakeContextSave(NAppHost::IServiceContext& serviceCtx, TLogContext logContext);

}  // namespace NAlice::NCuttlefish::NAppHostServices

