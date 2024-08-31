#pragma once

#include <alice/hollywood/library/global_context/global_context.h>

#include <library/cpp/neh/rpc.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NHollywood {

void GetVersionHttpHandle(TGlobalContext& /* globalContext */, const NNeh::IRequestRef& req);

void GetVersionAppHostHandle(NAppHost::IServiceContext& ctx, TGlobalContext& /* globalContext */);

// The path should look like `/hollywood_<shard_name>/utility/<intent>`
void UtilityHandler(NAppHost::IServiceContext& ctx, TGlobalContext& /* globalContext */);

void DumpSolomonCountersHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req);

void GetFastDataVersionHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req);

void ReloadFastDataHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req);

void ReopenLogsHandle(TGlobalContext& globalContext, const NNeh::IRequestRef& req);

void PingHandle(TGlobalContext& /* globalContext */, const NNeh::IRequestRef& req);


} // namespace NAlice::NHollywood
